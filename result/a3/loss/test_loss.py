

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

# ——— 数据：Payload vs 丢包率 ———
payload_len = np.array([16, 32, 64, 128, 256, 512, 1024, 1472])
# usr_loss = np.array([70, 77, 80, 86, 90, 95, 97, 98])
# xdp_loss = np.array([2.9, 2.7, 3.5, 2.7, 4.3, 7.6, 11, 13])

usr_loss = np.array([33.1, 47.9, 63.71, 77.61,87.11 , 93.12, 96.23,97.35])
xdp_loss = np.array([0.71, 0.72, 0.82, 0.56, 1.30, 1.10, 1.90, 1.10])

# 丢包率数据 (%)，请补全 xdp_loss 中的缺失值，使其长度与 payload_len 相同
# usr_loss = np.array([51, 51, 51, 57, 71, 81, 86, 93])
# xdp_loss = np.array([0.71, 0.72, 0.82, 0.56, 1.30, 1.10, 1.90, 1.10])

# ——— 计算“sqrt 坐标” ———
x_pos = np.sqrt(payload_len)

# ——— 绘制丢包率对比 ———
plt.figure(figsize=(8, 5), dpi=100)

# 用 x_pos 作为横坐标，带标记点
plt.plot(x_pos, xdp_loss, marker='o', label='XDP 丢包率', linewidth=2)
plt.plot(x_pos, usr_loss, marker='s', label='用户态 丢包率', linewidth=2)

# ——— 在每个点标注具体数值 ———
for x, y in zip(x_pos, xdp_loss):
    # XDP 丢包率一般较小，标注在点上方
    plt.text(x, y + max(xdp_loss)*0.02, f"{y:.2f}%", 
             ha='center', va='bottom', fontsize=9)

for x, y in zip(x_pos, usr_loss):
    # 用户态丢包率较大，标注在点下方
    plt.text(x, y - max(usr_loss)*0.02, f"{y:.0f}%", 
             ha='center', va='top', fontsize=9)

plt.title("丢包率对比", fontsize=14)
plt.xlabel("Payload 大小（字节）", fontsize=12)
plt.ylabel("丢包率 (%)", fontsize=12)

# 设置刻度：位置为 x_pos，标签为原 payload_len
ax = plt.gca()
ax.set_xticks(x_pos)
ax.set_xticklabels([str(int(p)) for p in payload_len])

plt.grid(True, linestyle='--', alpha=0.6)
plt.legend(fontsize=10)
plt.tight_layout()

# 保存图像
out_path = os.path.join(output_dir, "loss_vs_payload.png")
plt.savefig(out_path, dpi=300)
plt.close()

print(f"图已保存到 {out_path}")
