import matplotlib
import matplotlib.pyplot as plt
import seaborn as sns
from matplotlib import font_manager
import os
import shutil

# 确保输出文件夹存在
output_dir = "./sm4/img/test3"
os.makedirs(output_dir, exist_ok=True)

# 1. 完全重置 matplotlib 配置
matplotlib.rcParams.update(matplotlib.rcParamsDefault)

# 2. 清除 matplotlib 缓存
cache_dir = matplotlib.get_cachedir()
shutil.rmtree(cache_dir, ignore_errors=True)
print(f"已清除缓存目录: {cache_dir}")

# 3. 设置中文字体 - 使用绝对路径确保可靠性
try:
    font_paths = [
        r"C:\Windows\Fonts\msyh.ttc",   # 微软雅黑
        r"C:\Windows\Fonts\simhei.ttf", # 黑体
        r"E:\Download\stsong_ufonts.com_.ttf"  # 备用宋体
    ]
    font_path = None
    for fp in font_paths:
        if os.path.exists(fp):
            font_path = fp
            break

    if font_path:
        font_prop = font_manager.FontProperties(fname=font_path)
        font_manager.fontManager.addfont(font_path)
        plt.rcParams['font.family'] = font_prop.get_name()
        plt.rcParams['axes.unicode_minus'] = False
        print(f"成功加载字体: {font_path}")
    else:
        plt.rcParams['font.family'] = 'Microsoft YaHei'
        plt.rcParams['axes.unicode_minus'] = False
        print("使用系统默认中文字体")
except Exception as e:
    print(f"字体设置错误: {str(e)}")
    plt.rcParams['font.family'] = 'sans-serif'

print("当前使用字体:", plt.rcParams['font.family'])
print("可用中文字体:", [f.name for f in font_manager.fontManager.ttflist if 'hei' in f.name.lower() or 'ya' in f.name.lower() or 'song' in f.name.lower()])

# 更新后的发包数量和 payload 长度
send_counts = [100, 1000, 10000, 100000]
payload_len = [15, 510, 1020, 1470]

# 更新后的数据字典：data1 按 payload 组织
data1 = {
    15: {
        "xdp": {
            "throughput": [33254, 28667, 29069, 28885],
            "min": [126, 116, 115, 114],
            "avg": [487.37, 523.45, 515.97, 519.22],
            "max": [4396, 7274, 10014, 11776],
            "std": [765.09, 847.84, 850.20, 852.23],
            "jitter": [43.12, 7.17, 0.99, 0.12],
            "cpu": [13950, 143987, 1419760, 14340921],
            "mem": [25, 25, 38, 114],
            "recv": [6100, 61191, 612066, 6125392],
            "send": [5700, 57000, 570018, 5700347],
        },
        "user": {
            "throughput": [29191, 23375, 24390, 25404],
            "min": [187, 173, 164, 154],
            "avg": [541.63, 645.28, 615.25, 590.27],
            "max": [6791, 10688, 14088, 21884],
            "std": [931.03, 1043.89, 1064.04, 1031.35],
            "jitter": [66.71, 10.53, 1.39, 0.22],
            "cpu": [14221, 136870, 1417764, 14311151],
            "mem": [25, 25, 38, 114],
            "recv": [6006, 60021, 600195, 6002201],
            "send": [5704, 57008, 570026, 5700311],
        },
    },
    510: {
        "xdp": {
            "throughput": [1005991, 964644, 973038, 962128],
            "min": [124, 118, 116, 115],
            "avg": [534.08, 529.44, 524.17, 530.04],
            "max": [4403, 8186, 9784, 12056],
            "std": [768.03, 854.23, 841.70, 849.92],
            "jitter": [43.23, 8.08, 0.97, 0.12],
            "cpu": [14075, 136440, 1412648, 14024554],
            "mem": [25, 51, 38, 114],
            "recv": [55400, 554115, 5542855, 55420805],
            "send": [55200, 552004, 5520019, 55200282],
        },
        "user": {
            "throughput": [885909, 825307, 841052, 842896],
            "min": [315, 296, 284, 272],
            "avg": [585.07, 620.49, 606.39, 605.02],
            "max": [4195, 7669, 36142, 34986],
            "std": [592.77, 631.79, 845.73, 715.18],
            "jitter": [39.19, 7.38, 3.59, 0.35],
            "cpu": [14264, 129016, 1352100, 13650745],
            "mem": [12, 25, 38, 114],
            "recv": [55406, 554006, 5540571, 55401812],
            "send": [55200, 552004, 5520560, 55200361],
        },
    },
    1020: {
        "xdp": {
            "throughput": [2242669, 1801012, 1843586, 1848053],
            "min": [156, 144, 133, 124],
            "avg": [483.55, 635.60, 553.21, 551.82],
            "max": [3768, 112780, 14074, 19766],
            "std": [721.21, 4129.09, 920.81, 938.51],
            "jitter": [36.48, 112.75, 1.39, 0.20],
            "cpu": [14607, 132474, 1334937, 13301990],
            "mem": [25, 25, 38, 114],
            "recv": [106600, 1066184, 10664198, 106617900],
            "send": [106200, 1062000, 10620013, 106200295],
        },
        "user": {
            "throughput": [1286637, 1151635, 1270412, 1192846],
            "min": [426, 417, 388, 385],
            "avg": [805.61, 889.57, 804.09, 859.19],
            "max": [12550, 32140, 84923, 27391],
            "std": [1404.07, 1700.05, 1939.47, 1348.28],
            "jitter": [122.47, 31.75, 8.45, 0.27],
            "cpu": [14626, 137293, 1385880, 14876719],
            "mem": [25, 25, 38, 114],
            "recv": [106600, 1066036, 10660237, 106604542],
            "send": [106200, 1062004, 10620794, 106202982],
        },
    },
    1470: {
        "xdp": {
            "throughput": [3054395, 2645217, 2891352, 2618464],
            "min": [160, 148, 138, 134],
            "avg": [506.88, 557.06, 508.36, 561.38],
            "max": [4142, 6280, 6530, 19268],
            "std": [742.35, 806.17, 839.00, 925.27],
            "jitter": [40.22, 6.14, 0.64, 0.19],
            "cpu": [14252, 122177, 1357559, 13263481],
            "mem": [25, 25, 51, 114],
            "recv": [151400, 1514000, 15140012, 151423810],
            "send": [151200, 1512000, 15120013, 151200257],
        },
        "user": {
            "throughput": [1206709, 1133102, 1204390, 1092655],
            "min": [544, 493, 459, 441],
            "avg": [1223.76, 1304.36, 1220.72, 1359.45],
            "max": [18521, 36641, 27985, 101351],
            "std": [2499.40, 2831.30, 2485.23, 2637.45],
            "jitter": [181.59, 36.18, 2.75, 1.01],
            "cpu": [15863, 153855, 1525222, 16430531],
            "mem": [12, 25, 38, 114],
            "recv": [151412, 1514000, 15142145, 151427385],
            "send": [151204, 1512000, 15120609, 151203137],
        },
    },
}

# 更新后的数据字典：data2 按发包数量组织（payload 顺序：[15, 510, 1020, 1470]）
data2 = {
    100: {
        "xdp": {
            "throughput": [33254, 1005991, 2242669, 3054395],
            "min": [126, 124, 156, 160],
            "avg": [487.37, 534.08, 483.55, 506.88],
            "max": [4396, 4403, 3768, 4142],
            "std": [765.09, 768.03, 721.21, 742.35],
            "jitter": [43.12, 43.23, 36.48, 40.22],
            "cpu": [13950, 14075, 14607, 14252],
            "mem": [25, 25, 25, 25],
            "recv": [6100, 55400, 106600, 151400],
            "send": [5700, 55200, 106200, 151200],
        },
        "user": {
            "throughput": [29191, 885909, 1286637, 1206709],
            "min": [187, 315, 426, 544],
            "avg": [541.63, 585.07, 805.61, 1223.76],
            "max": [6791, 4195, 12550, 18521],
            "std": [931.03, 592.77, 1404.07, 2499.40],
            "jitter": [66.71, 39.19, 122.47, 181.59],
            "cpu": [14221, 14264, 14626, 15863],
            "mem": [25, 12, 25, 12],
            "recv": [6006, 55406, 106600, 151412],
            "send": [5704, 55200, 106200, 151204],
        },
    },
    1000: {
        "xdp": {
            "throughput": [28667, 964644, 1801012, 2645217],
            "min": [116, 118, 144, 148],
            "avg": [523.45, 529.44, 635.60, 557.06],
            "max": [7274, 8186, 112780, 6280],
            "std": [847.84, 854.23, 4129.09, 806.17],
            "jitter": [7.17, 8.08, 112.75, 6.14],
            "cpu": [143987, 136440, 132474, 122177],
            "mem": [25, 51, 25, 25],
            "recv": [61191, 554115, 1066184, 1514000],
            "send": [57000, 552004, 1062000, 1512000],
        },
        "user": {
            "throughput": [23375, 825307, 1151635, 1133102],
            "min": [173, 296, 417, 493],
            "avg": [645.28, 620.49, 889.57, 1304.36],
            "max": [10688, 7669, 32140, 36641],
            "std": [1043.89, 631.79, 1700.05, 2831.30],
            "jitter": [10.53, 7.38, 31.75, 36.18],
            "cpu": [136870, 129016, 137293, 153855],
            "mem": [25, 25, 25, 25],
            "recv": [60021, 554006, 1066036, 1514000],
            "send": [57008, 552004, 1062004, 1512000],
        },
    },
    10000: {
        "xdp": {
            "throughput": [29069, 973038, 1843586, 2891352],
            "min": [115, 116, 133, 138],
            "avg": [515.97, 524.17, 553.21, 508.36],
            "max": [10014, 9784, 14074, 6530],
            "std": [850.20, 841.70, 920.81, 839.00],
            "jitter": [0.99, 0.97, 1.39, 0.64],
            "cpu": [1419760, 1412648, 1334937, 1357559],
            "mem": [38, 38, 38, 51],
            "recv": [612066, 5542855, 10664198, 15140012],
            "send": [570018, 5520019, 10620013, 15120013],
        },
        "user": {
            "throughput": [24390, 841052, 1270412, 1204390],
            "min": [164, 284, 388, 459],
            "avg": [615.25, 606.39, 804.09, 1220.72],
            "max": [14088, 36142, 84923, 27985],
            "std": [1064.04, 845.73, 1939.47, 2485.23],
            "jitter": [1.39, 3.59, 8.45, 2.75],
            "cpu": [1417764, 1352100, 1385880, 1525222],
            "mem": [38, 38, 38, 38],
            "recv": [600195, 5540571, 10660237, 15142145],
            "send": [570026, 5520560, 10620794, 15120609],
        },
    },
    100000: {
        "xdp": {
            "throughput": [28885, 962128, 1848053, 2618464],
            "min": [114, 115, 124, 134],
            "avg": [519.22, 530.04, 551.82, 561.38],
            "max": [11776, 12056, 19766, 19268],
            "std": [852.23, 849.92, 938.51, 925.27],
            "jitter": [0.12, 0.12, 0.20, 0.19],
            "cpu": [14340921, 14024554, 13301990, 13263481],
            "mem": [114, 114, 114, 114],
            "recv": [6125392, 55420805, 106617900, 151423810],
            "send": [5700347, 55200282, 106200295, 151200257],
        },
        "user": {
            "throughput": [25404, 842896, 1192846, 1092655],
            "min": [154, 272, 385, 441],
            "avg": [590.27, 605.02, 859.19, 1359.45],
            "max": [21884, 34986, 27391, 101351],
            "std": [1031.35, 715.18, 1348.28, 2637.45],
            "jitter": [0.22, 0.35, 0.27, 1.01],
            "cpu": [14311151, 13650745, 14876719, 16430531],
            "mem": [114, 114, 114, 114],
            "recv": [6002201, 55401812, 106604542, 151427385],
            "send": [5700311, 55200361, 106202982, 151203137],
        },
    },
}

# ---------- 绘图函数 ----------

# 修改后的绘图函数，采用 2×2 子图排列
def draw_by_send_counts(metric_name, ylabel, filename, key):
    font_prop = font_manager.FontProperties(fname=font_path) if font_path and os.path.exists(font_path) else None
    # 改为 2 行 2 列的子图排列，确保总数为4
    fig, axs = plt.subplots(2, 2, figsize=(12, 10), dpi=100)
    
    for i, plen in enumerate(payload_len):
        row = i // 2
        col = i % 2
        xdp_data = data1[plen]["xdp"][key]
        user_data = data1[plen]["user"][key]

        axs[row][col].plot(send_counts, xdp_data, marker='o', markersize=8, label="XDP", linewidth=2, color='#1f77b4')
        axs[row][col].plot(send_counts, user_data, marker='s', markersize=8, label="用户态", linewidth=2, color='#ff7f0e')
        
        axs[row][col].set_title(f"Payload = {plen} 字节", fontproperties=font_prop, fontsize=12, pad=10)
        axs[row][col].set_xlabel("发包数量", fontproperties=font_prop, fontsize=10)
        axs[row][col].set_ylabel(ylabel, fontproperties=font_prop, fontsize=10)
        axs[row][col].set_xscale('log')
        axs[row][col].grid(True, linestyle='--', alpha=0.6)
        axs[row][col].legend(prop=font_prop, fontsize=9)
    
    plt.suptitle(metric_name, fontproperties=font_prop, fontsize=14, y=1.02)
    plt.tight_layout()
    plt.savefig(filename, bbox_inches='tight', dpi=300)
    plt.close()

def draw_by_payload(metric_name, ylabel, filename, key):
    font_prop = font_manager.FontProperties(fname=font_path) if font_path and os.path.exists(font_path) else None
    send_count_list = list(data2.keys())
    # 子图排列仍然为 2×2
    fig, axs = plt.subplots(2, 2, figsize=(12, 10), dpi=100)
    
    for idx, count in enumerate(send_count_list):
        row = idx // 2
        col = idx % 2
        xdp_data = data2[count]["xdp"][key]
        user_data = data2[count]["user"][key]

        axs[row][col].plot(payload_len, xdp_data, marker='o', markersize=8, label="XDP", linewidth=2, color='#1f77b4')
        axs[row][col].plot(payload_len, user_data, marker='s', markersize=8, label="用户态", linewidth=2, color='#ff7f0e')
        
        axs[row][col].set_title(f"发包数量 = {count}", fontproperties=font_prop, fontsize=11, pad=10)
        axs[row][col].set_xlabel("Payload 长度（字节）", fontproperties=font_prop, fontsize=9)
        axs[row][col].set_ylabel(ylabel, fontproperties=font_prop, fontsize=9)
        axs[row][col].set_xticks(payload_len)
        axs[row][col].grid(True, linestyle='--', alpha=0.6)
        axs[row][col].legend(prop=font_prop, fontsize=9)
    
    plt.suptitle(metric_name, fontproperties=font_prop, fontsize=14, y=1.02)
    plt.tight_layout()
    plt.savefig(filename, bbox_inches='tight', dpi=300)
    plt.close()

# ---------- 生成所有图表 ----------

# 以发包数量为横坐标（data1）的图表，均采用 2×2 子图排列
draw_by_send_counts("XDP VS 用户态 吞吐量对比", "吞吐量（B/s）", os.path.join(output_dir, "图1_XDP_vs_用户态_吞吐量.png"), "throughput")
draw_by_send_counts("XDP VS 用户态 平均延迟对比", "平均延迟（us）", os.path.join(output_dir, "图2_XDP_vs_用户态_平均延迟.png"), "avg")
draw_by_send_counts("XDP VS 用户态 最大延迟对比", "最大延迟（us）", os.path.join(output_dir, "图3_XDP_vs_用户态_最大延迟.png"), "max")
draw_by_send_counts("XDP VS 用户态 最小延迟对比", "最小延迟（us）", os.path.join(output_dir, "图4_XDP_vs_用户态_最小延迟.png"), "min")

# 以发包数量为横坐标（新指标）
draw_by_send_counts("XDP VS 用户态 标准差对比", "标准差（us）", os.path.join(output_dir, "图5_XDP_vs_用户态_标准差.png"), "std")
draw_by_send_counts("XDP VS 用户态 抖动对比", "抖动（us）", os.path.join(output_dir, "图6_XDP_vs_用户态_抖动.png"), "jitter")
draw_by_send_counts("XDP VS 用户态 CPU使用时间对比", "CPU使用时间（us）", os.path.join(output_dir, "图7_XDP_vs_用户态_CPU.png"), "cpu")
draw_by_send_counts("XDP VS 用户态 内存使用对比", "内存使用（KB）", os.path.join(output_dir, "图8_XDP_vs_用户态_内存.png"), "mem")
draw_by_send_counts("XDP VS 用户态 接受字节对比", "接收字节（B）", os.path.join(output_dir, "图9_XDP_vs_用户态_接收字节.png"), "recv")
draw_by_send_counts("XDP VS 用户态 发送字节对比", "发送字节（B）", os.path.join(output_dir, "图10_XDP_vs_用户态_发送字节.png"), "send")

# 以 payload 为横坐标（data2）的图表，均采用 2×2 子图排列
draw_by_payload("XDP VS 用户态 吞吐量对比（不同发包数量）", "吞吐量（B/s）", os.path.join(output_dir, "图11_XDP_vs_用户态_吞吐量.png"), "throughput")
draw_by_payload("XDP VS 用户态 平均延迟对比（不同发包数量）", "平均延迟（us）", os.path.join(output_dir, "图12_XDP_vs_用户态_平均延迟.png"), "avg")
draw_by_payload("XDP VS 用户态 最小延迟对比（不同发包数量）", "最小延迟（us）", os.path.join(output_dir, "图13_XDP_vs_用户态_最小延迟.png"), "min")
draw_by_payload("XDP VS 用户态 最大延迟对比（不同发包数量）", "最大延迟（us）", os.path.join(output_dir, "图14_XDP_vs_用户态_最大延迟.png"), "max")
draw_by_payload("XDP VS 用户态 标准差对比（不同发包数量）", "标准差（us）", os.path.join(output_dir, "图15_XDP_vs_用户态_标准差.png"), "std")
draw_by_payload("XDP VS 用户态 抖动对比（不同发包数量）", "抖动（us）", os.path.join(output_dir, "图16_抖动.png"), "jitter")
draw_by_payload("XDP VS 用户态 CPU使用时间对比（不同发包数量）", "CPU使用时间（us）", os.path.join(output_dir, "图17_XDP_vs_用户态_CPU.png"), "cpu")
draw_by_payload("XDP VS 用户态 内存使用对比（不同发包数量）", "内存使用（KB）", os.path.join(output_dir, "图18_XDP_vs_用户态_内存.png"), "mem")
draw_by_payload("XDP VS 用户态 接受字节对比（不同发包数量）", "接收字节（B）", os.path.join(output_dir, "图19_XDP_vs_用户态_接收字节.png"), "recv")
draw_by_payload("XDP VS 用户态 发送字节对比（不同发包数量）", "发送字节（B）", os.path.join(output_dir, "图20_XDP_vs_用户态_发送字节.png"), "send")

print("所有图表已生成完毕！")
