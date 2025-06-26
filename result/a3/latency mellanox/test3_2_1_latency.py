
import math 
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from matplotlib import ticker, font_manager
import os
import shutil

# ——— 输出目录 ———
output_dir = "./sm4/img/test3"
os.makedirs(output_dir, exist_ok=True)

# ——— 重置 & 清缓存 ———
matplotlib.rcParams.update(matplotlib.rcParamsDefault)
shutil.rmtree(matplotlib.get_cachedir(), ignore_errors=True)

# ——— 字体设置 ———
try:
    font_paths = [
        r"C:/Windows/Fonts/msyh.ttc",
        r"C:/Windows/Fonts/simhei.ttf",
        r"E:/Download/stsong_ufonts.com_.ttf"
    ]
    for fp in font_paths:
        if os.path.exists(fp):
            prop = font_manager.FontProperties(fname=fp)
            font_manager.fontManager.addfont(fp)
            plt.rcParams['font.family'] = prop.get_name()
            plt.rcParams['axes.unicode_minus'] = False
            break
    else:
        plt.rcParams['font.family'] = 'Microsoft YaHei'
        plt.rcParams['axes.unicode_minus'] = False
except:
    plt.rcParams['font.family'] = 'sans-serif'

# ——— 数据：发包数量固定 10000，平均延迟 vs Payload ———
payload_len = [16, 32, 64, 128, 256, 512, 1024, 1472]
xdp_avg =    [30.62, 35.06, 39.16, 39.29, 41.16, 54.08, 74.76, 88.16]
user_avg =   [32.01, 47.55, 53.52, 61.28, 62.66, 80.2, 119.38, 162.43]

# 非线性递增示例数据 (µs)
# xdp_avg  = [205.12, 230.45, 267.89, 312.34, 376.78, 429.56, 481.23, 512.87]
# user_avg = [520.33, 645.27, 789.41, 912.58, 1134.76, 1288.95, 1421.67, 1498.42]


# ——— 计算“sqrt 坐标” ———
x_pos = [math.sqrt(p) for p in payload_len]

# ——— 绘制平均延迟对比 ———
plt.figure(figsize=(8,5), dpi=100)
# 用 x_pos 作为横坐标
plt.plot(x_pos, xdp_avg, marker='o', label='XDP 平均延迟', linewidth=2)
plt.plot(x_pos, user_avg, marker='s', label='用户态 平均延迟', linewidth=2)

# ——— 在每个点标注具体数值 ———
for x, y in zip(x_pos, xdp_avg):
    plt.text(x, y - 7, f"{y:.2f}", ha='center', va='bottom', fontsize=9)  # 标注在 XDP 点下方
for x, y in zip(x_pos, user_avg):
    plt.text(x, y + max(user_avg)*0.01, f"{y:.2f}", ha='center', va='bottom', fontsize=9)

plt.title("平均延迟对比 (发包数量=10000)", fontsize=14)
plt.xlabel("Payload 长度（字节）", fontsize=12)
plt.ylabel("平均延迟 (µs)", fontsize=12)

# 将刻度位置设为 x_pos，但标签仍然显示原 payload_len
ax = plt.gca()
ax.set_xticks(x_pos)
ax.set_xticklabels([str(int(p)) for p in payload_len])

plt.grid(True, linestyle='--', alpha=0.6)
plt.legend(fontsize=10)
plt.tight_layout()

# 保存图像
plt.savefig(os.path.join(output_dir, "avg_delay_comparison.png"), dpi=300)
plt.close()

print(f"图已保存到 {os.path.join(output_dir, 'avg_delay_comparison.png')}")