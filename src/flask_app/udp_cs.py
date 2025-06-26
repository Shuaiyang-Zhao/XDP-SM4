
import socket
import time
import threading
import statistics
import logging
from collections import deque
import subprocess
import os
from datetime import datetime

# 调试模式标志 - 设置为True启用日志和调试，False关闭
DEBUG_MODE = False
# 性能监控标志 - 设置为True启用性能监控，包括系统资源收集
PERFORMANCE_MONITOR = True  # 即使在非调试模式下也可以收集性能数据


# 日志配置函数
def setup_logging():
    if not DEBUG_MODE:
        # 如果不是调试模式，将日志级别设为ERROR（只记录错误）
        logging.getLogger().setLevel(logging.ERROR)
        return

    # 以下是调试模式的日志配置
    logger = logging.getLogger()
    logger.setLevel(logging.INFO)

    # 检查是否已经有处理器，避免重复添加
    if not logger.handlers:
        # 文件处理器
        file_handler = logging.FileHandler('system_performance.log')
        file_handler.setLevel(logging.INFO)
        file_formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
        file_handler.setFormatter(file_formatter)
        logger.addHandler(file_handler)

        # 控制台处理器
        console_handler = logging.StreamHandler()
        console_handler.setLevel(logging.INFO)
        console_formatter = logging.Formatter('[%(asctime)s] %(message)s', datefmt='%H:%M:%S')
        console_handler.setFormatter(console_formatter)
        logger.addHandler(console_handler)


# 初始化日志
setup_logging()


# 调试打印函数
def debug_print_bytes(label, data: bytes):
    if DEBUG_MODE:
        print(f"{label}: {' '.join(f'{b:02x}' for b in data)}")


# 调试日志函数
def debug_log(message, level=logging.INFO):
    if DEBUG_MODE:
        logging.log(level, message)


LOCAL_IP = '192.168.58.132'
ENCRYPT_IP = '192.168.58.129'
ENCRYPT_PORT = 1234
LOCAL_PORT_ENC = 1235
DECRYPT_IP = '192.168.58.128'
DECRYPT_PORT = 1233
LOCAL_PORT_DEC = 1236
TIMEOUT_SECONDS = 5
SM4_BLOCK_SIZE = 16

# 用于保存上次加密耗时
encrypt_time = 0

# 性能指标存储 - 修改为分别存储加密和解密服务器的资源数据
performance_data = {
    'latency_history': deque(maxlen=20),
    'encrypt_time_history': deque(maxlen=20),
    'decrypt_time_history': deque(maxlen=20),
    'throughput_history': deque(maxlen=20),
    'packet_sizes': deque(maxlen=20),
    'encrypt_cpu_usage': deque(maxlen=20),  # 加密服务器CPU使用率
    'encrypt_memory_usage': deque(maxlen=20),  # 加密服务器内存使用率
    'decrypt_cpu_usage': deque(maxlen=20),  # 解密服务器CPU使用率
    'decrypt_memory_usage': deque(maxlen=20),  # 解密服务器内存使用率
    'total_packets': 0,
    'successful_packets': 0,
}

performance_lock = threading.Lock()
# 用于同步等待资源数据的事件
encrypt_resource_event = threading.Event()
decrypt_resource_event = threading.Event()


def send_and_receive(payload: bytes, local_port: int, server_ip: str, server_port: int):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    start = time.time()
    try:
        sock.bind((LOCAL_IP, local_port))
        sock.settimeout(TIMEOUT_SECONDS)
        debug_print_bytes("发送数据包", payload)
        sock.sendto(payload, (server_ip, server_port))
        data, _ = sock.recvfrom(4096)
        debug_print_bytes("收到数据", data)
        return data, time.time() - start
    finally:
        sock.close()


def query_remote_usage(ip):
    """
    通过调用query_remote_usage.py脚本查询远程服务器CPU和内存使用率
    """
    try:
        # 调用query_remote_usage.py脚本
        script_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "query_remote_usage.py")
        result = subprocess.check_output(["python3", script_path, ip],
                                         stderr=subprocess.STDOUT,
                                         universal_newlines=True)
        # 解析输出，获取CPU和内存使用率
        parts = result.strip().split()
        if len(parts) == 2 and parts[0] != '-1':
            cpu_usage = float(parts[0])
            mem_usage = float(parts[1])
            return cpu_usage, mem_usage
    except subprocess.CalledProcessError:
        if DEBUG_MODE:
            logging.error(f"调用query_remote_usage.py脚本失败，IP: {ip}")
    except Exception as e:
        if DEBUG_MODE:
            logging.error(f"查询远程服务器资源使用率失败: {e}")
    return None, None


# 使用线程同步查询服务器资源使用情况，等待结果返回
def sync_query_usage(server_ip, is_encrypt_server):
    server_type = "加密服务器" if is_encrypt_server else "解密服务器"
    debug_log(f"开始查询{server_type} ({server_ip}) 资源使用情况")

    cpu, mem = query_remote_usage(server_ip)
    if cpu is not None:
        with performance_lock:
            if is_encrypt_server:
                performance_data['encrypt_cpu_usage'].append(cpu)
                performance_data['encrypt_memory_usage'].append(mem)
                debug_log(f"{server_type}资源使用情况 - CPU: {cpu:.2f}%, 内存: {mem:.2f}%")
                encrypt_resource_event.set()
            else:
                performance_data['decrypt_cpu_usage'].append(cpu)
                performance_data['decrypt_memory_usage'].append(mem)
                debug_log(f"{server_type}资源使用情况 - CPU: {cpu:.2f}%, 内存: {mem:.2f}%")
                decrypt_resource_event.set()
    else:
        if DEBUG_MODE:
            logging.warning(f"未能获取{server_type} ({server_ip}) 资源使用情况")
        if is_encrypt_server:
            encrypt_resource_event.set()
        else:
            decrypt_resource_event.set()

    return cpu, mem


def encrypt(plain_str: str):
    global performance_data, encrypt_time
    # 重置加密资源事件
    if PERFORMANCE_MONITOR:
        encrypt_resource_event.clear()

    # 文本→UTF-8 bytes→PKCS#7 填充
    raw = plain_str.encode('utf-8')
    pad = SM4_BLOCK_SIZE - (len(raw) % SM4_BLOCK_SIZE) or SM4_BLOCK_SIZE
    raw += bytes([pad]) * pad

    debug_log(f"正在加密数据 - 原始大小: {len(plain_str)} 字节, 填充后: {len(raw)} 字节")

    # 先完成加密过程并计时
    response, proc = send_and_receive(raw, LOCAL_PORT_ENC, ENCRYPT_IP, ENCRYPT_PORT)

    # 记录加密性能
    debug_log(f"加密完成 - 耗时: {proc * 1000:.2f} ms, 数据大小: {len(raw)} 字节")

    with performance_lock:
        encrypt_time = proc
        performance_data['encrypt_time_history'].append(proc * 1000)
        performance_data['packet_sizes'].append(len(raw))

    # 启动线程查询加密服务器资源
    if PERFORMANCE_MONITOR:
        thread = threading.Thread(target=sync_query_usage, args=(ENCRYPT_IP, True))
        thread.daemon = True
        thread.start()

    cipher_hex = response.hex()
    debug_log(f"加密测试 - 密文: {cipher_hex}, 耗时: {proc * 1000:.2f} ms, 大小: {len(raw)} 字节")

    # 等待资源数据查询完成，超时5秒
    if PERFORMANCE_MONITOR:
        encrypt_resource_event.wait(5)

        # 输出加密服务器资源使用情况
        if DEBUG_MODE and len(performance_data['encrypt_cpu_usage']) > 0 and len(
                performance_data['encrypt_memory_usage']) > 0:
            cpu = performance_data['encrypt_cpu_usage'][-1]
            mem = performance_data['encrypt_memory_usage'][-1]
            debug_log(f"加密过程系统资源使用情况 - 加密服务器CPU: {cpu:.2f}%, 内存: {mem:.2f}%")

    return cipher_hex, proc, len(raw)


def decrypt(cipher_hex: str):
    global performance_data, encrypt_time
    # 重置解密资源事件
    if PERFORMANCE_MONITOR:
        decrypt_resource_event.clear()

    payload = bytes.fromhex(cipher_hex)

    debug_log(f"正在解密数据 - 密文大小: {len(payload)} 字节")

    # 先完成解密过程并计时
    response, proc = send_and_receive(payload, LOCAL_PORT_DEC, DECRYPT_IP, DECRYPT_PORT)

    # 去 PKCS#7 填充、UTF-8 解码
    pad = response[-1]
    body = response[:-pad]
    text = body.decode('utf-8')

    total = encrypt_time + proc
    tp = (len(payload) * 8) / total / 1024 if total > 0 else 0

    # 记录解密性能
    debug_log(f"解密完成 - 解密耗时: {proc * 1000:.2f} ms, 总耗时: {total * 1000:.2f} ms, 吞吐量: {tp:.2f} Kbps")

    with performance_lock:
        performance_data['decrypt_time_history'].append(proc * 1000)
        performance_data['latency_history'].append(total * 1000)
        performance_data['throughput_history'].append(tp)
        performance_data['total_packets'] += 1
        performance_data['successful_packets'] += 1

    # 在完成解密处理后，启动线程查询解密服务器资源
    if PERFORMANCE_MONITOR:
        thread = threading.Thread(target=sync_query_usage, args=(DECRYPT_IP, False))
        thread.daemon = True
        thread.start()

    debug_log(f"解密测试 - 明文: {text}, 总耗时: {total * 1000:.2f} ms, 吞吐量: {tp:.2f} Kbps")

    # 等待资源数据查询完成，超时5秒
    if PERFORMANCE_MONITOR:
        decrypt_resource_event.wait(5)

        # 输出解密服务器资源使用情况
        if DEBUG_MODE and len(performance_data['decrypt_cpu_usage']) > 0 and len(
                performance_data['decrypt_memory_usage']) > 0:
            cpu = performance_data['decrypt_cpu_usage'][-1]
            mem = performance_data['decrypt_memory_usage'][-1]
            debug_log(f"解密过程系统资源使用情况 - 解密服务器CPU: {cpu:.2f}%, 内存: {mem:.2f}%")

    return text, total, tp


def get_performance_stats():
    with performance_lock:
        if not performance_data['latency_history']:
            if DEBUG_MODE:
                logging.warning("尚未收集到性能数据")
            raise Exception("尚未收集到性能数据")

        lat = list(performance_data['latency_history'])
        enc = list(performance_data['encrypt_time_history'])
        dec = list(performance_data['decrypt_time_history'])
        thr = list(performance_data['throughput_history'])

        # 构造带 current/min/max/avg 的结构
        stats = {
            'latency': {
                'current': lat[-1],
                'min': min(lat),
                'max': max(lat),
                'avg': statistics.mean(lat),
                'history': lat,
            },
            'encrypt_time': {
                'current': enc[-1] if enc else None,
                'history': enc,
            },
            'decrypt_time': {
                'current': dec[-1] if dec else None,
                'history': dec,
            },
            'throughput': {
                'current': thr[-1] if thr else None,
                'history': thr,
                'unit': 'Kbps'
            },
            'system': {
                # 分别返回加密和解密服务器的资源使用情况
                'encrypt_cpu_usage': list(performance_data['encrypt_cpu_usage']),
                'encrypt_memory_usage': list(performance_data['encrypt_memory_usage']),
                'decrypt_cpu_usage': list(performance_data['decrypt_cpu_usage']),
                'decrypt_memory_usage': list(performance_data['decrypt_memory_usage']),
            },
            'packets': {
                'total': performance_data['total_packets'],
                'successful': performance_data['successful_packets'],
                'success_rate': (performance_data['successful_packets'] / performance_data['total_packets'] * 100) if
                performance_data['total_packets'] > 0 else 0
            }
        }

        # 记录性能统计数据
        if DEBUG_MODE:
            logging.info("---------- 性能统计数据 ----------")
            logging.info(
                f"延迟 - 当前: {lat[-1]:.2f} ms, 最小: {min(lat):.2f} ms, 最大: {max(lat):.2f} ms, 平均: {statistics.mean(lat):.2f} ms")
            logging.info(f"加密时间 - 当前: {enc[-1]:.2f} ms")
            logging.info(f"解密时间 - 当前: {dec[-1]:.2f} ms")
            logging.info(f"吞吐量 - 当前: {thr[-1]:.2f} Kbps")

            if performance_data['encrypt_cpu_usage']:
                logging.info(
                    f"加密服务器 - CPU: {performance_data['encrypt_cpu_usage'][-1]:.2f}%, 内存: {performance_data['encrypt_memory_usage'][-1]:.2f}%")

            if performance_data['decrypt_cpu_usage']:
                logging.info(
                    f"解密服务器 - CPU: {performance_data['decrypt_cpu_usage'][-1]:.2f}%, 内存: {performance_data['decrypt_memory_usage'][-1]:.2f}%")

            logging.info(
                f"数据包 - 总数: {performance_data['total_packets']}, 成功: {performance_data['successful_packets']}, 成功率: {stats['packets']['success_rate']:.2f}%")
            logging.info("------------------------------------")

        return stats


if __name__ == '__main__':
    debug_log("=== 系统测试启动 ===")

    # 简单测试
    c, t1, s = encrypt("Hello 中文")
    d, t2, tp = decrypt(c)

    # 输出简洁摘要
    debug_log("=== 系统测试结束 ===")
    if DEBUG_MODE:
        print("C:", c)
        print("D:", d, "TP:", tp)


# import socket
# import time
# import threading
# import statistics
# import logging
# from collections import deque
# import subprocess
# import os
# from datetime import datetime
#
# # 调试模式标志 - 设置为True启用日志和调试，False关闭
# DEBUG_MODE = False
# # 性能监控标志 - 设置为True启用性能监控，包括系统资源收集
# PERFORMANCE_MONITOR = True  # 即使在非调试模式下也可以收集性能数据
#
#
# # 日志配置函数
# def setup_logging():
#     if not DEBUG_MODE:
#         # 如果不是调试模式，将日志级别设为ERROR（只记录错误）
#         logging.getLogger().setLevel(logging.ERROR)
#         return
#
#     # 以下是调试模式的日志配置
#     logger = logging.getLogger()
#     logger.setLevel(logging.INFO)
#
#     # 检查是否已经有处理器，避免重复添加
#     if not logger.handlers:
#         # 文件处理器
#         file_handler = logging.FileHandler('system_performance.log')
#         file_handler.setLevel(logging.INFO)
#         file_formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
#         file_handler.setFormatter(file_formatter)
#         logger.addHandler(file_handler)
#
#         # 控制台处理器
#         console_handler = logging.StreamHandler()
#         console_handler.setLevel(logging.INFO)
#         console_formatter = logging.Formatter('[%(asctime)s] %(message)s', datefmt='%H:%M:%S')
#         console_handler.setFormatter(console_formatter)
#         logger.addHandler(console_handler)
#
#
# # 初始化日志
# setup_logging()
#
#
# # 调试打印函数
# def debug_print_bytes(label, data: bytes):
#     if DEBUG_MODE:
#         print(f"{label}: {' '.join(f'{b:02x}' for b in data)}")
#
#
# # 调试日志函数
# def debug_log(message, level=logging.INFO):
#     if DEBUG_MODE:
#         logging.log(level, message)
#
#
# LOCAL_IP = '192.168.58.132'
# ENCRYPT_IP = '192.168.58.129'
# ENCRYPT_PORT = 1234
# LOCAL_PORT_ENC = 1235
# DECRYPT_IP = '192.168.58.128'
# DECRYPT_PORT = 1233
# LOCAL_PORT_DEC = 1236
# TIMEOUT_SECONDS = 5
# SM4_BLOCK_SIZE = 16
#
# # 用于保存上次加密耗时
# encrypt_time = 0
#
# # 性能指标存储 - 修改为分别存储加密和解密服务器的资源数据
# performance_data = {
#     'latency_history': deque(maxlen=20),
#     'encrypt_time_history': deque(maxlen=20),
#     'decrypt_time_history': deque(maxlen=20),
#     'throughput_history': deque(maxlen=20),
#     'packet_sizes': deque(maxlen=20),
#     'encrypt_cpu_usage': deque(maxlen=20),  # 加密服务器CPU使用率
#     'encrypt_memory_usage': deque(maxlen=20),  # 加密服务器内存使用率
#     'decrypt_cpu_usage': deque(maxlen=20),  # 解密服务器CPU使用率
#     'decrypt_memory_usage': deque(maxlen=20),  # 解密服务器内存使用率
#     'total_packets': 0,
#     'successful_packets': 0,
# }
#
# performance_lock = threading.Lock()
# # 用于同步等待资源数据的事件
# encrypt_resource_event = threading.Event()
# decrypt_resource_event = threading.Event()
#
# # 控制资源监控的标志
# encrypt_monitor_active = False
# decrypt_monitor_active = False
#
#
# def send_and_receive(payload: bytes, local_port: int, server_ip: str, server_port: int):
#     sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
#     sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
#     start = time.time()
#     try:
#         sock.bind((LOCAL_IP, local_port))
#         sock.settimeout(TIMEOUT_SECONDS)
#         debug_print_bytes("发送数据包", payload)
#         sock.sendto(payload, (server_ip, server_port))
#         data, _ = sock.recvfrom(4096)
#         debug_print_bytes("收到数据", data)
#         return data, time.time() - start
#     finally:
#         sock.close()
#
#
# def query_remote_usage(ip):
#     """
#     通过调用query_remote_usage.py脚本查询远程服务器CPU和内存使用率
#     """
#     try:
#         # 调用query_remote_usage.py脚本
#         script_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "query_remote_usage.py")
#         result = subprocess.check_output(["python3", script_path, ip],
#                                          stderr=subprocess.STDOUT,
#                                          universal_newlines=True)
#         # 解析输出，获取CPU和内存使用率
#         parts = result.strip().split()
#         if len(parts) == 2 and parts[0] != '-1':
#             cpu_usage = float(parts[0])
#             mem_usage = float(parts[1])
#             return cpu_usage, mem_usage
#     except subprocess.CalledProcessError:
#         if DEBUG_MODE:
#             logging.error(f"调用query_remote_usage.py脚本失败，IP: {ip}")
#     except Exception as e:
#         if DEBUG_MODE:
#             logging.error(f"查询远程服务器资源使用率失败: {e}")
#     return None, None
#
#
# # 用于持续监控服务器资源使用情况的函数
# def continuous_resource_monitor(server_ip, is_encrypt_server):
#     global encrypt_monitor_active, decrypt_monitor_active
#
#     server_type = "加密服务器" if is_encrypt_server else "解密服务器"
#     debug_log(f"开始持续监控{server_type} ({server_ip}) 资源使用情况")
#
#     # 检查是否应该继续监控
#     while (is_encrypt_server and encrypt_monitor_active) or (not is_encrypt_server and decrypt_monitor_active):
#         try:
#             cpu, mem = query_remote_usage(server_ip)
#             if cpu is not None:
#                 with performance_lock:
#                     if is_encrypt_server:
#                         performance_data['encrypt_cpu_usage'].append(cpu)
#                         performance_data['encrypt_memory_usage'].append(mem)
#                         debug_log(f"{server_type}资源使用情况 - CPU: {cpu:.2f}%, 内存: {mem:.2f}%")
#                     else:
#                         performance_data['decrypt_cpu_usage'].append(cpu)
#                         performance_data['decrypt_memory_usage'].append(mem)
#                         debug_log(f"{server_type}资源使用情况 - CPU: {cpu:.2f}%, 内存: {mem:.2f}%")
#             else:
#                 if DEBUG_MODE:
#                     logging.warning(f"未能获取{server_type} ({server_ip}) 资源使用情况")
#
#             # 短暂休眠，避免频繁查询
#             time.sleep(0.1)  # 100ms间隔，可以根据需要调整
#
#         except Exception as e:
#             if DEBUG_MODE:
#                 logging.error(f"监控{server_type}资源时出错: {e}")
#
#     debug_log(f"结束{server_type} ({server_ip}) 资源监控")
#
#
# # 启动加密服务器资源监控
# def start_encrypt_monitor():
#     global encrypt_monitor_active
#     if not PERFORMANCE_MONITOR:
#         return
#
#     encrypt_monitor_active = True
#     thread = threading.Thread(target=continuous_resource_monitor, args=(ENCRYPT_IP, True))
#     thread.daemon = True
#     thread.start()
#     debug_log("启动加密服务器资源监控")
#
#
# # 停止加密服务器资源监控
# def stop_encrypt_monitor():
#     global encrypt_monitor_active
#     if not PERFORMANCE_MONITOR:
#         return
#
#     encrypt_monitor_active = False
#     debug_log("停止加密服务器资源监控")
#
#
# # 启动解密服务器资源监控
# def start_decrypt_monitor():
#     global decrypt_monitor_active
#     if not PERFORMANCE_MONITOR:
#         return
#
#     decrypt_monitor_active = True
#     thread = threading.Thread(target=continuous_resource_monitor, args=(DECRYPT_IP, False))
#     thread.daemon = True
#     thread.start()
#     debug_log("启动解密服务器资源监控")
#
#
# # 停止解密服务器资源监控
# def stop_decrypt_monitor():
#     global decrypt_monitor_active
#     if not PERFORMANCE_MONITOR:
#         return
#
#     decrypt_monitor_active = False
#     debug_log("停止解密服务器资源监控")
#
#
# def encrypt(plain_str: str):
#     global performance_data, encrypt_time
#
#     # 启动加密服务器资源监控
#     start_encrypt_monitor()
#
#     # 文本→UTF-8 bytes→PKCS#7 填充
#     raw = plain_str.encode('utf-8')
#     pad = SM4_BLOCK_SIZE - (len(raw) % SM4_BLOCK_SIZE) or SM4_BLOCK_SIZE
#     raw += bytes([pad]) * pad
#
#     debug_log(f"正在加密数据 - 原始大小: {len(plain_str)} 字节, 填充后: {len(raw)} 字节")
#
#     # 先完成加密过程并计时
#     response, proc = send_and_receive(raw, LOCAL_PORT_ENC, ENCRYPT_IP, ENCRYPT_PORT)
#
#     # 停止加密服务器资源监控
#     stop_encrypt_monitor()
#
#     # 记录加密性能
#     debug_log(f"加密完成 - 耗时: {proc * 1000:.2f} ms, 数据大小: {len(raw)} 字节")
#
#     with performance_lock:
#         encrypt_time = proc
#         performance_data['encrypt_time_history'].append(proc * 1000)
#         performance_data['packet_sizes'].append(len(raw))
#
#     cipher_hex = response.hex()
#     debug_log(f"加密测试 - 密文: {cipher_hex}, 耗时: {proc * 1000:.2f} ms, 大小: {len(raw)} 字节")
#
#     # 输出加密服务器资源使用情况
#     if DEBUG_MODE and PERFORMANCE_MONITOR and performance_data['encrypt_cpu_usage']:
#         cpu = performance_data['encrypt_cpu_usage'][-1]
#         mem = performance_data['encrypt_memory_usage'][-1]
#         debug_log(f"加密过程系统资源使用情况 - 加密服务器CPU: {cpu:.2f}%, 内存: {mem:.2f}%")
#
#     return cipher_hex, proc, len(raw)
#
#
# def decrypt(cipher_hex: str):
#     global performance_data, encrypt_time
#
#     # 启动解密服务器资源监控
#     start_decrypt_monitor()
#
#     payload = bytes.fromhex(cipher_hex)
#
#     debug_log(f"正在解密数据 - 密文大小: {len(payload)} 字节")
#
#     # 先完成解密过程并计时
#     response, proc = send_and_receive(payload, LOCAL_PORT_DEC, DECRYPT_IP, DECRYPT_PORT)
#
#     # 停止解密服务器资源监控
#     stop_decrypt_monitor()
#
#     # 去 PKCS#7 填充、UTF-8 解码
#     pad = response[-1]
#     body = response[:-pad]
#     text = body.decode('utf-8')
#
#     total = encrypt_time + proc
#     tp = (len(payload) * 8) / total / 1024 if total > 0 else 0
#
#     # 记录解密性能
#     debug_log(f"解密完成 - 解密耗时: {proc * 1000:.2f} ms, 总耗时: {total * 1000:.2f} ms, 吞吐量: {tp:.2f} Kbps")
#
#     with performance_lock:
#         performance_data['decrypt_time_history'].append(proc * 1000)
#         performance_data['latency_history'].append(total * 1000)
#         performance_data['throughput_history'].append(tp)
#         performance_data['total_packets'] += 1
#         performance_data['successful_packets'] += 1
#
#     debug_log(f"解密测试 - 明文: {text}, 总耗时: {total * 1000:.2f} ms, 吞吐量: {tp:.2f} Kbps")
#
#     # 输出解密服务器资源使用情况
#     if DEBUG_MODE and PERFORMANCE_MONITOR and performance_data['decrypt_cpu_usage']:
#         cpu = performance_data['decrypt_cpu_usage'][-1]
#         mem = performance_data['decrypt_memory_usage'][-1]
#         debug_log(f"解密过程系统资源使用情况 - 解密服务器CPU: {cpu:.2f}%, 内存: {mem:.2f}%")
#
#     return text, total, tp
#
#
# def get_performance_stats():
#     with performance_lock:
#         if not performance_data['latency_history']:
#             if DEBUG_MODE:
#                 logging.warning("尚未收集到性能数据")
#             raise Exception("尚未收集到性能数据")
#
#         lat = list(performance_data['latency_history'])
#         enc = list(performance_data['encrypt_time_history'])
#         dec = list(performance_data['decrypt_time_history'])
#         thr = list(performance_data['throughput_history'])
#
#         # 构造带 current/min/max/avg 的结构
#         stats = {
#             'latency': {
#                 'current': lat[-1],
#                 'min': min(lat),
#                 'max': max(lat),
#                 'avg': statistics.mean(lat),
#                 'history': lat,
#             },
#             'encrypt_time': {
#                 'current': enc[-1] if enc else None,
#                 'history': enc,
#             },
#             'decrypt_time': {
#                 'current': dec[-1] if dec else None,
#                 'history': dec,
#             },
#             'throughput': {
#                 'current': thr[-1] if thr else None,
#                 'history': thr,
#                 'unit': 'Kbps'
#             },
#             'system': {
#                 # 分别返回加密和解密服务器的资源使用情况
#                 'encrypt_cpu_usage': list(performance_data['encrypt_cpu_usage']),
#                 'encrypt_memory_usage': list(performance_data['encrypt_memory_usage']),
#                 'decrypt_cpu_usage': list(performance_data['decrypt_cpu_usage']),
#                 'decrypt_memory_usage': list(performance_data['decrypt_memory_usage']),
#             },
#             'packets': {
#                 'total': performance_data['total_packets'],
#                 'successful': performance_data['successful_packets'],
#                 'success_rate': (performance_data['successful_packets'] / performance_data['total_packets'] * 100) if
#                 performance_data['total_packets'] > 0 else 0
#             }
#         }
#
#         # 记录性能统计数据
#         if DEBUG_MODE:
#             logging.info("---------- 性能统计数据 ----------")
#             logging.info(
#                 f"延迟 - 当前: {lat[-1]:.2f} ms, 最小: {min(lat):.2f} ms, 最大: {max(lat):.2f} ms, 平均: {statistics.mean(lat):.2f} ms")
#             logging.info(f"加密时间 - 当前: {enc[-1]:.2f} ms")
#             logging.info(f"解密时间 - 当前: {dec[-1]:.2f} ms")
#             logging.info(f"吞吐量 - 当前: {thr[-1]:.2f} Kbps")
#
#             if performance_data['encrypt_cpu_usage']:
#                 logging.info(
#                     f"加密服务器 - CPU: {performance_data['encrypt_cpu_usage'][-1]:.2f}%, 内存: {performance_data['encrypt_memory_usage'][-1]:.2f}%")
#
#             if performance_data['decrypt_cpu_usage']:
#                 logging.info(
#                     f"解密服务器 - CPU: {performance_data['decrypt_cpu_usage'][-1]:.2f}%, 内存: {performance_data['decrypt_memory_usage'][-1]:.2f}%")
#
#             logging.info(
#                 f"数据包 - 总数: {performance_data['total_packets']}, 成功: {performance_data['successful_packets']}, 成功率: {stats['packets']['success_rate']:.2f}%")
#             logging.info("------------------------------------")
#
#         return stats
#
#
# if __name__ == '__main__':
#     debug_log("=== 系统测试启动 ===")
#
#     # 简单测试
#     c, t1, s = encrypt("Hello 中文")
#     d, t2, tp = decrypt(c)
#
#     # 输出简洁摘要
#     debug_log("=== 系统测试结束 ===")
#     if DEBUG_MODE:
#         print("C:", c)
#         print("D:", d, "TP:", tp)
