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

# ——— 数据：Payload vs 吞吐量 ———
payload_len = np.array([16, 32, 64, 128, 256, 512, 1024, 1472])

# 吞吐量数据 (Mbps)
# xdp_tp = np.array([5.78, 11.37, 22.29, 43.34, 71.79, 122.93, 241.70, 310.29])
# usr_tp = np.array([4.67, 9.05, 17.45, 32.88, 53.96, 92.25, 124.01, 136.23])

xdp_tp = np.array([16.7, 31.4, 66.9, 134, 262, 488, 929, 1413.12 ])
usr_tp = np.array([2.49, 4.70, 9.85, 18.6, 33.3, 53.5, 83.6, 86.5])

# ——— 计算“sqrt 坐标” ———
x_pos = np.sqrt(payload_len)

# ——— 绘制吞吐量对比 ———
plt.figure(figsize=(8, 5), dpi=100)

# 用 x_pos 作为横坐标，带标记点
plt.plot(x_pos, xdp_tp, marker='o', label='XDP 吞吐量', linewidth=2)
plt.plot(x_pos, usr_tp, marker='s', label='用户态 吞吐量', linewidth=2)

# ——— 在每个点标注具体数值 ———
for x, y in zip(x_pos, xdp_tp):
    plt.text(x, y + max(xdp_tp)*0.01, f"{y:.2f}", ha='center', va='bottom', fontsize=9)
for x, y in zip(x_pos, usr_tp):
    plt.text(x, y - max(usr_tp)*0.03, f"{y:.2f}", ha='center', va='top', fontsize=9)

plt.title("吞吐量对比", fontsize=14)
plt.xlabel("Payload 大小（字节）", fontsize=12)
plt.ylabel("吞吐量 (Mbps)", fontsize=12)

# 将刻度位置设为 x_pos，但标签仍然显示原 payload_len
ax = plt.gca()
ax.set_xticks(x_pos)
ax.set_xticklabels([str(int(p)) for p in payload_len])

plt.grid(True, linestyle='--', alpha=0.6)
plt.legend(fontsize=10)
plt.tight_layout()

# 保存图像
plt.savefig(os.path.join(output_dir, "throughput_vs_payload_new.png"), dpi=300)
plt.close()

print(f"图已保存到 {os.path.join(output_dir, 'throughput_vs_payload_new.png')}")