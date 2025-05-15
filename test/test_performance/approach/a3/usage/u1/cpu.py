import paramiko
import time
import csv
import os

REMOTE_IP = "192.168.58.128"
USERNAME = "zsy"
PASSWORD = "020302"
INTERVAL = 1  # 秒
CSV_FILE = "cpu_mem_detailed_usage.csv"

def create_ssh_connection():
    """建立 SSH 连接"""
    while True:
        try:
            print(f"正在连接 {REMOTE_IP} ...")
            ssh = paramiko.SSHClient()
            ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            ssh.connect(REMOTE_IP, username=USERNAME, password=PASSWORD)
            print("✅ SSH 连接成功！开始实时采集...按 Ctrl+C 退出")
            return ssh
        except Exception as e:
            print(f"[连接失败] {e}，5秒后重试...")
            time.sleep(5)

def main():
    # 打开 CSV 文件
    file_exists = os.path.isfile(CSV_FILE)
    csv_file = open(CSV_FILE, mode='a', newline='')
    csv_writer = csv.writer(csv_file)
    if not file_exists:
        csv_writer.writerow(["时间戳", "CPU使用率(%)", "CPU空闲率(%)",
                              "总内存(MB)", "已用内存(MB)", "空闲内存(MB)", "内存使用率(%)"])

    ssh = create_ssh_connection()

    # 更新后的远程采集命令
    command = """
CPU_LINE=$(top -b -n1 | grep 'Cpu(s)');
CPU_IDLE=$(echo "$CPU_LINE" | awk '{print $8}');
CPU_USAGE=$(echo "scale=2; 100 - $CPU_IDLE" | bc);
MEM_LINE=$(free -m | grep Mem);
TOTAL_MEM=$(echo $MEM_LINE | awk '{print $2}');
USED_MEM=$(echo $MEM_LINE | awk '{print $3}');
FREE_MEM=$(echo $MEM_LINE | awk '{print $4}');
MEM_USAGE=$(echo "scale=2; $USED_MEM/$TOTAL_MEM*100" | bc);
echo "$CPU_USAGE $CPU_IDLE $TOTAL_MEM $USED_MEM $FREE_MEM $MEM_USAGE"
"""

    try:
        while True:
            try:
                stdin, stdout, stderr = ssh.exec_command(command, timeout=5)
                output = stdout.read().decode().strip()

                if output and len(output.split()) == 6:
                    cpu_usage, cpu_idle, total_mem, used_mem, free_mem, mem_usage = output.split()
                    now = time.strftime("%Y-%m-%d %H:%M:%S")
                    # 只打印精简版
                    print(f"[{now}] CPU使用率: {cpu_usage}%, 内存使用率: {mem_usage}%")
                    # 写详细到 CSV
                    csv_writer.writerow([now, cpu_usage, cpu_idle,
                                         total_mem, used_mem, free_mem, mem_usage])
                    csv_file.flush()
                else:
                    print("[警告] 输出格式异常，跳过一次")

                time.sleep(INTERVAL)

            except Exception as inner_err:
                print(f"[采集错误] {inner_err}，尝试重新连接 SSH...")
                ssh.close()
                ssh = create_ssh_connection()

    except KeyboardInterrupt:
        print("\n⛔️ 手动退出采集。")
    finally:
        ssh.close()
        csv_file.close()
        print("✅ 已安全关闭连接和文件。")

if __name__ == "__main__":
    main()
