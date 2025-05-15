import os
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib import rcParams
from matplotlib.animation import FuncAnimation
from matplotlib import font_manager
import shutil
import matplotlib

# 1. 完全重置 matplotlib 配置
matplotlib.rcParams.update(matplotlib.rcParamsDefault)

# 2. 清除 matplotlib 缓存
cache_dir = matplotlib.get_cachedir()
shutil.rmtree(cache_dir, ignore_errors=True)
print(f"已清除缓存目录: {cache_dir}")

# 3. 设置中文字体 - 使用绝对路径确保可靠性
try:
    font_paths = [
        r"/home/test-machine/sm4/udp_receive/SimHei.ttf",   # 微软雅黑
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

# 自动清理 CSV 文件的逻辑
csv_file = "realtime_stats.csv"
if os.path.exists(csv_file):
    # 可以选择清空文件内容，或者删除文件
    # 清空文件内容
    open(csv_file, 'w').close()
    print(f"已清理 CSV 文件: {csv_file}")
    # 或者删除文件并重新创建一个空的 CSV 文件
    # os.remove(csv_file)
    # print(f"已删除 CSV 文件: {csv_file}")

plt.style.use('ggplot')  # 替代样式

def animate(i):
    try:
        df = pd.read_csv(csv_file, header=None)
        df.columns = ["time", "avg", "min", "max", "p95", "p99", "stddev", "jitter", "throughput"]

        x = df["time"]

        # 清除上一次的绘制
        axs[0].clear()
        axs[0].plot(x, df["avg"], label='平均延迟')
        axs[0].plot(x, df["min"], label='最小延迟')
        axs[0].plot(x, df["max"], label='最大延迟')
        axs[0].legend()
        axs[0].set_title("延迟 (us)")

        axs[1].clear()
        axs[1].plot(x, df["p95"], label='P95 延迟')
        axs[1].plot(x, df["p99"], label='P99 延迟')
        axs[1].legend()
        axs[1].set_title("P95 / P99 延迟 (us)")

        axs[2].clear()
        axs[2].plot(x, df["stddev"], label='标准差')
        axs[2].plot(x, df["jitter"], label='抖动')
        axs[2].legend()
        axs[2].set_title("抖动与标准差 (us)")

        axs[3].clear()
        axs[3].plot(x, df["throughput"], label='吞吐率 (MBps)', color='tab:orange')
        axs[3].legend()
        axs[3].set_title("吞吐率 (MBps)")

        # 设置坐标轴标签
        for ax in axs:
            ax.set_xlabel("测试时间 (s)")
            ax.set_ylabel("数值")

    except Exception as e:
        print("等待数据中...", e)

# 创建4个子图 (三个延迟相关图和一个吞吐率图)
fig, axs = plt.subplots(4, 1, figsize=(10, 10), tight_layout=True)

# 更新动画
ani = FuncAnimation(fig, animate, interval=1000)

# 显示图形
plt.show()
