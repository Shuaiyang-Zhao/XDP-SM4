import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os
import shutil
import matplotlib
from matplotlib import font_manager

# === 1. 清理 matplotlib 缓存并恢复默认设置 ===
matplotlib.rcParams.update(matplotlib.rcParamsDefault)
cache_dir = matplotlib.get_cachedir()
shutil.rmtree(cache_dir, ignore_errors=True)
print(f"✅ 已清除缓存目录: {cache_dir}")

# === 2. 设置中文字体（SimHei）===
try:
    font_path = "/home/test-machine/sm4/udp_receive/SimHei.ttf"
    if os.path.exists(font_path):
        font_prop = font_manager.FontProperties(fname=font_path)
        font_manager.fontManager.addfont(font_path)
        font_name = font_prop.get_name()
        plt.rcParams['font.family'] = font_name
        matplotlib.rcParams['axes.unicode_minus'] = False
        print(f"✅ 成功加载字体: {font_path} ({font_name})")
    else:
        print("⚠️ 字体文件不存在，使用默认字体")
except Exception as e:
    print(f"字体设置错误: {e}")
    plt.rcParams['font.family'] = 'sans-serif'

# === 3. 加载数据 ===
filenames = [
    "latency_16.csv", "latency_32.csv", "latency_64.csv",
    "latency_128.csv", "latency_256.csv", "latency_512.csv",
    "latency_1024.csv", "latency_1472.csv"
]

dfs = []
for fname in filenames:
    if os.path.exists(fname):
        df = pd.read_csv(fname)
        dfs.append(df)
    else:
        print(f"⚠️ 未找到文件: {fname}")

# 合并所有数据
all_data = pd.concat(dfs, ignore_index=True)

# === 4. 绘图 ===
plt.figure(figsize=(12, 6))
ax = plt.gca()

# 箱线图
all_data.boxplot(
    by="负载长度(字节)",
    column="时延(微秒)",
    ax=ax,
    grid=True,
    patch_artist=True,
    boxprops=dict(facecolor="lightblue"),
    medianprops=dict(color='red'),
    flierprops=dict(marker='o', markersize=5, linestyle='none', markerfacecolor='gray'),
)

# === 5. 标注 P95、最大值、平均值 ===
grouped = all_data.groupby("负载长度(字节)")
xticks = sorted(all_data["负载长度(字节)"].unique())
for i, length in enumerate(xticks):
    group = grouped.get_group(length)["时延(微秒)"]
    p95 = np.percentile(group, 95)
    max_val = group.max()
    avg_val = group.mean()

    # P95
    plt.scatter([i + 1], [p95], color="green", s=100, marker="*", zorder=5)
    # 最大值注释
    plt.text(i + 1, max_val + 10, f"最大: {int(max_val)}μs", ha='center', va='bottom', fontsize=9)
    # 平均值注释
    plt.text(i + 1, avg_val, f"Avg: {int(avg_val)}", ha='center', va='bottom', fontsize=9)

# === 6. 图像美化 ===
plt.title("网络加密通信时延箱线图", fontsize=16, fontproperties=font_prop)
plt.suptitle("")  # 移除 Pandas 自动添加的副标题
plt.xlabel("负载长度（字节）", fontsize=12, fontproperties=font_prop)
plt.ylabel("时延（微秒）", fontsize=12, fontproperties=font_prop)
plt.xticks(ticks=np.arange(1, len(xticks) + 1), labels=[str(x) for x in xticks])

plt.tight_layout()

# === 7. 保存并展示 ===
output_file = "latency_boxplot_default.png"
plt.savefig(output_file, dpi=300)
print(f"✅ 图像已保存为 {output_file}")

plt.show()
