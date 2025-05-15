import matplotlib
import matplotlib.pyplot as plt
import seaborn as sns
from matplotlib import font_manager
import os
import shutil

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

# 用你提供的数据替换以下两段
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
}  # 你的 data2 数据

# 绘图函数定义
def plot_comparison(data, x_data, x_label, y_labels, titles, filename, subplot_titles, legend=True, log_x=False):
    fig, axes = plt.subplots(1, 3, figsize=(18, 5))
    plt.suptitle(titles['main'])
    for i, key in enumerate(subplot_titles):
        ax = axes[i]
        ax.plot(x_data, data[key]['xdp'], marker='o', label='XDP')
        ax.plot(x_data, data[key]['user'], marker='s', label='User')
        if log_x:
            ax.set_xscale('log')
        ax.set_xlabel(x_label)
        ax.set_ylabel(y_labels)
        ax.set_title(f"{titles['sub']} {key}")
        if legend:
            ax.legend()
    plt.tight_layout()
    plt.savefig(filename)
    plt.close()

# 图1-4：横坐标为发包数量
metrics = ['throughput', 'avg', 'max', 'min']
figure_names = [
    ('吞吐量对比', 'figure1_throughput_vs_sendcount.png'),
    ('平均延迟对比', 'figure2_avg_latency_vs_sendcount.png'),
    ('最大延迟对比', 'figure3_max_latency_vs_sendcount.png'),
    ('最小延迟对比', 'figure4_min_latency_vs_sendcount.png')
]

for metric, (title, filename) in zip(metrics, figure_names):
    plot_data = {
        plen: {
            'xdp': data1[plen]['xdp'][metric],
            'user': data1[plen]['user'][metric]
        } for plen in payload_len
    }
    plot_comparison(
        data=plot_data,
        x_data=send_counts,
        x_label='发包数量',
        y_labels=title.split(' ')[0],
        titles={'main': f'XDP vs User {title}（横坐标：发包数量）', 'sub': '负载长度'},
        filename=filename,
        subplot_titles=payload_len,
        log_x=True
    )

# 图5-8：横坐标为负载长度
send_counts_data2 = list(data2.keys())
metrics = ['throughput', 'avg', 'max', 'min']
figure_names = [
    ('吞吐量对比', 'figure5_throughput_vs_payloadlen.png'),
    ('平均延迟对比', 'figure6_avg_latency_vs_payloadlen.png'),
    ('最大延迟对比', 'figure7_max_latency_vs_payloadlen.png'),
    ('最小延迟对比', 'figure8_min_latency_vs_payloadlen.png')
]

for metric, (title, filename) in zip(metrics, figure_names):
    fig, axes = plt.subplots(2, 3, figsize=(20, 12))
    plt.suptitle(f'XDP vs User {title}（横坐标：负载长度）')
    for i, scount in enumerate(send_counts_data2):
        row, col = divmod(i, 3)
        ax = axes[row, col]
        try:
            ax.plot(payload_len, data2[scount]['xdp'][metric], marker='o', label='XDP')
            ax.plot(payload_len, data2[scount]['user'][metric], marker='s', label='User')
        except KeyError:
            print(f"警告：{metric} 数据不存在于 send_count={scount}")
            continue
        ax.set_xlabel('负载长度 (bytes)')
        ax.set_xticks(payload_len)
        ax.set_ylabel(title.split(' ')[0])
        ax.set_title(f'发包数量: {scount}')
        ax.legend()
    plt.tight_layout()
    plt.savefig(filename)
    plt.close()

print("所有图表已生成完毕！")