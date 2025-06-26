import matplotlib
import matplotlib.pyplot as plt
import seaborn as sns
from matplotlib import font_manager
import os
import shutil

import os

# 确保文件夹存在
output_dir = "./sm4/img/test1"
os.makedirs(output_dir, exist_ok=True)


# 1. 完全重置matplotlib配置
matplotlib.rcParams.update(matplotlib.rcParamsDefault)

# 2. 清除matplotlib缓存
cache_dir = matplotlib.get_cachedir()
shutil.rmtree(cache_dir, ignore_errors=True)
print(f"已清除缓存目录: {cache_dir}")

# 3. 设置中文字体 - 使用绝对路径确保可靠性
try:
    # 尝试多种可能的字体路径
    font_paths = [
        r"C:\Windows\Fonts\msyh.ttc",  # 微软雅黑
        r"C:\Windows\Fonts\simhei.ttf",  # 黑体
        r"E:\Download\stsong_ufonts.com_.ttf"  # 你之前可用的宋体
    ]
    
    font_path = None
    for fp in font_paths:
        if os.path.exists(fp):
            font_path = fp
            break
    
    if font_path:
        # 注册字体
        font_prop = font_manager.FontProperties(fname=font_path)
        font_manager.fontManager.addfont(font_path)
        plt.rcParams['font.family'] = font_prop.get_name()
        plt.rcParams['axes.unicode_minus'] = False
        print(f"成功加载字体: {font_path}")
    else:
        # 回退到系统字体
        plt.rcParams['font.family'] = 'Microsoft YaHei'
        plt.rcParams['axes.unicode_minus'] = False
        print("使用系统默认中文字体")
except Exception as e:
    print(f"字体设置错误: {str(e)}")
    plt.rcParams['font.family'] = 'sans-serif'

# 4. 验证字体配置
print("当前使用字体:", plt.rcParams['font.family'])
print("可用中文字体:", [f.name for f in font_manager.fontManager.ttflist if 'hei' in f.name.lower() or 'ya' in f.name.lower() or 'song' in f.name.lower()])

# 5. 设置全局样式
# sns.set_style("whitegrid", {'font.sans-serif': [plt.rcParams['font.family']]})
sns.set_style("whitegrid", {'font.sans-serif': plt.rcParams['font.family']})
send_counts = [100, 1000, 10000, 100000, 500000, 1000000]
payload_len = [15, 510, 1470]


# [此处保留你原有的data1和data2数据字典]
data1 = {
    15: {
        "xdp": {
            "throughput": [41865, 30715, 29979, 29640, 29952, 29633],
            "min": [114, 115, 113, 113, 113, 113],
            "avg": [358.23, 488.32, 500.28, 506.00, 500.73, 506.12],
            "max": [6203, 12539, 11730, 14595, 20139, 29553],
        },
        "user": {
            "throughput": [31422, 26015, 25986, 25806, 26049, 25989],
            "min": [181, 189, 172, 168, 160, 154],
            "avg": [477.30, 576.51, 577.16, 581.17, 575.75, 577.10],
            "max": [2367, 5685, 15402, 15748, 17831, 846819],
        },
    },
    510: {
        "xdp": {
            "throughput": [1543817, 1067311, 1001209, 996674, 1035194, 872237],
            "min": [151, 124, 123, 120, 118, 120],
            "avg": [330.32, 477.78, 509.31, 511.64, 492.59, 584.62],
            "max": [1622, 6858, 16576, 19536, 562162, 44554],
        },
        "user": {
            "throughput": [690495, 688105, 695110, 698899, 691682, 678393],
            "min": [326, 314, 287, 262, 256, 258],
            "avg": [738.54, 741.06, 733.61, 729.63, 737.23, 751.67],
            "max": [9193, 21308, 35269, 28946, 97009, 78425],
        },
    },
    1470: {
        "xdp": {
            "throughput": [4232042, 2804669, 2554763, 2684310, 2738914, 2571291],
            "min": [161, 155, 143, 133, 123, 124],
            "avg": [347.30, 524.06, 575.32, 547.54, 536.64, 571.63],
            "max": [3464, 6025, 15646, 88657, 21757, 54247],
        },
        "user": {
            "throughput": [1131604, 1048072, 1111674, 1065441, 1106578, 1035038],
            "min": [597, 553, 500, 468, 451, 467],
            "avg": [1298.93, 1402.45, 1322.21, 1379.58, 1328.29, 1420.11],
            "max": [15955, 21109, 26367, 34181, 256537, 642833],
        },
    },
}  # 你的 data1 数据
data2 = {
      100: {
        "xdp": {
            "throughput": [41865, 1543817, 4232042],
            "min": [114, 151, 161],
            "avg": [358.23, 330.32, 347.30],
            "max": [6203, 1622, 3464],
        },
        "user": {
            "throughput": [31422, 690495, 1131604],
            "min": [181, 326, 597],
            "avg": [477.30, 738.54, 1298.93],
            "max": [2367, 9193, 15955],
        }
    },
    1000: {
        "xdp": {
            "throughput": [30715, 1067311, 2804669],
            "min": [115, 124, 155],
            "avg": [488.32, 477.78, 524.06],
            "max": [12539, 6858, 6025],
        },
        "user": {
            "throughput": [26015, 688105, 1048072],
            "min": [189, 314, 553],
            "avg": [576.51, 741.06, 1402.45],
            "max": [5685, 21308, 21109],
        }
    },
    10000: {
        "xdp": {
            "throughput": [29979, 1001209, 2554763],
            "min": [113, 123, 143],
            "avg": [500.28, 511.64, 575.32],
            "max": [11730, 16576, 15646],
        },
        "user": {
            "throughput": [25986, 695110, 1111674],
            "min": [172, 287, 500],
            "avg": [577.16, 733.61, 1322.21],
            "max": [15402, 35269, 26367],
        }
    },
    100000: {
        "xdp": {
            "throughput": [29640, 996674, 2684310],
            "min": [113, 120, 133],
            "avg": [506.00, 511.64, 547.54],
            "max": [14595, 19536, 88657],
        },
        "user": {
            "throughput": [25806, 698899, 1065441],
            "min": [168, 262, 468],
            "avg": [581.17, 729.63, 1379.58],
            "max": [15748, 28946, 34181],
        }
    },
    500000: {
        "xdp": {
            "throughput": [29952, 1035194, 2738914],
            "min": [113, 118, 123],
            "avg": [500.73, 492.59, 536.64],
            "max": [20139, 562162, 21757],
        },
        "user": {
            "throughput": [26049, 691682, 1106578],
            "min": [160, 256, 451],
            "avg": [575.75, 737.23, 1328.29],
            "max": [17831, 97009, 256537],
        }
    },
    1000000: {
        "xdp": {
            "throughput": [29633, 872237, 2571291],
            "min": [113, 120, 124],
            "avg": [506.12, 584.62, 571.63],
            "max": [29553, 44554, 54247],
        },
        "user": {
            "throughput": [25989, 678393, 1035038],
            "min": [154, 258, 467],
            "avg": [577.10, 751.67, 1420.11],
            "max": [846819, 78425, 642833], 
        }
        
    }
}  

# ---------- 优化后的绘图函数 ----------
def draw_by_send_counts(metric_name, ylabel, filename, key):
    # 创建字体属性对象
    font_prop = font_manager.FontProperties(fname=font_path) if font_path and os.path.exists(font_path) else None
    
    fig, axs = plt.subplots(1, 3, figsize=(18, 6), dpi=100)
    
    for i, plen in enumerate(payload_len):
        xdp_data = data1[plen]["xdp"][key]
        user_data = data1[plen]["user"][key]

        # 绘制数据
        axs[i].plot(send_counts, xdp_data, marker='o', markersize=8, label="XDP", linewidth=2, color='#1f77b4')
        axs[i].plot(send_counts, user_data, marker='s', markersize=8, label="User", linewidth=2, color='#ff7f0e')
        
        # 设置文本元素
        axs[i].set_title(f"Payload = {plen} 字节", 
                        fontproperties=font_prop, fontsize=12, pad=10)
        axs[i].set_xlabel("发包数量", 
                         fontproperties=font_prop, fontsize=10)
        axs[i].set_ylabel(ylabel, 
                         fontproperties=font_prop, fontsize=10)
        axs[i].set_xscale('log')
        axs[i].grid(True, linestyle='--', alpha=0.6)
        axs[i].legend(prop=font_prop, fontsize=9)
    
    plt.suptitle(metric_name, 
                fontproperties=font_prop, fontsize=14, y=1.02)
    plt.tight_layout()
    plt.savefig(filename, bbox_inches='tight', dpi=300)
    plt.close()

def draw_by_payload(metric_name, ylabel, filename, key):
    font_prop = font_manager.FontProperties(fname=font_path) if font_path and os.path.exists(font_path) else None
    
    fig, axs = plt.subplots(2, 3, figsize=(20, 12), dpi=100)
    send_count_list = list(data2.keys())
    
    for idx, count in enumerate(send_count_list):
        ax = axs[idx // 3][idx % 3]
        xdp_data = data2[count]["xdp"][key]
        user_data = data2[count]["user"][key]

        # 绘制数据
        ax.plot(payload_len, xdp_data, marker='o', markersize=8, label="XDP", linewidth=2, color='#1f77b4')
        ax.plot(payload_len, user_data, marker='s', markersize=8, label="用户态", linewidth=2, color='#ff7f0e')
        
        # 设置文本元素
        ax.set_title(f"发包数量 = {count}", 
                    fontproperties=font_prop, fontsize=11, pad=10)
        ax.set_xlabel("Payload 长度（字节）", 
                     fontproperties=font_prop, fontsize=9)
        ax.set_ylabel(ylabel, 
                     fontproperties=font_prop, fontsize=9)
        ax.set_xticks(payload_len)
        ax.grid(True, linestyle='--', alpha=0.6)
        ax.legend(prop=font_prop, fontsize=9)
    
    plt.suptitle(metric_name, 
                fontproperties=font_prop, fontsize=14, y=1.02)
    plt.tight_layout()
    plt.savefig(filename, bbox_inches='tight', dpi=300)
    plt.close()


# ---------- 生成所有图表 ----------
# 图1-4：以发包数量为横坐标
draw_by_send_counts("XDP VS User 吞吐量对比", "吞吐量（B/s）", os.path.join(output_dir, "图1_XDP_vs_User_吞吐量.png"), "throughput")
draw_by_send_counts("XDP VS User 平均延迟对比", "平均延迟（us）", os.path.join(output_dir, "图2_XDP_vs_User_平均延迟.png"), "avg")
draw_by_send_counts("XDP VS User 最大延迟对比", "最大延迟（us）", os.path.join(output_dir, "图3_XDP_vs_User_最大延迟.png"), "max")
draw_by_send_counts("XDP VS User 最小延迟对比", "最小延迟（us）", os.path.join(output_dir, "图4_XDP_vs_User_最小延迟.png"), "min")

# 图5-8：以payload为横坐标
draw_by_payload("XDP VS User 吞吐量对比（不同发包数量）", "吞吐量（B/s）", os.path.join(output_dir, "图5_XDP_vs_User_吞吐量_by_payload.png"), "throughput")
draw_by_payload("XDP VS User 平均延迟对比（不同发包数量）", "平均延迟（us）", os.path.join(output_dir, "图6_XDP_vs_User_平均延迟_by_payload.png"), "avg")
draw_by_payload("XDP VS User 最小延迟对比（不同发包数量）", "最小延迟（us）", os.path.join(output_dir, "图7_XDP_vs_User_最小延迟_by_payload.png"), "min")
draw_by_payload("XDP VS User 最大延迟对比（不同发包数量）", "最大延迟（us）", os.path.join(output_dir, "图8_XDP_vs_User_最大延迟_by_payload.png"), "max")

print("所有图表已生成完毕！")
