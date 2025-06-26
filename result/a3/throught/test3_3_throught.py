
import math 
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from matplotlib import ticker, font_manager
import os
import shutil

# ——— 输出目录 ———
output_dir = "./result/test3/throught/"
os.makedirs(output_dir, exist_ok=True)

# ——— 重置 & 清除 matplotlib 缓存 ———
matplotlib.rcParams.update(matplotlib.rcParamsDefault)
shutil.rmtree(matplotlib.get_cachedir(), ignore_errors=True)

# ——— 中文字体设置 ———
try:
    font_paths = [
        r"C:\Windows\Fonts\msyh.ttc",
        r"C:\Windows\Fonts\simhei.ttf",
        r"E:\Download\stsong_ufonts.com_.ttf"
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
xdp_tp = np.array([5.78, 11.37, 22.29, 43.34, 71.79, 122.93, 241.70, 310.29])
usr_tp = np.array([4.67, 9.05, 17.45, 32.88, 53.96, 92.25, 124.01, 136.23])


# ——— 将 payload 映射到 sqrt 空间，便于可视化非等距 ———
x_pos = np.sqrt(payload_len)

# ——— 绘制吞吐量对比 ———
plt.figure(figsize=(8, 5), dpi=100)

# 用 x_pos 作为横坐标，带标记点
plt.plot(x_pos, xdp_tp, marker='o', label='XDP 吞吐量', linewidth=2)
plt.plot(x_pos, usr_tp, marker='s', label='用户态 吞吐量', linewidth=2)

plt.title("吞吐量对比", fontsize=14)
plt.xlabel("Payload 大小（字节）", fontsize=12)
plt.ylabel("吞吐量 (Mbps)", fontsize=12)

# 自定义 X 轴刻度：位置用 sqrt(payload)，标签显示原始 payload
ax = plt.gca()
ax.set_xticks(x_pos)
ax.set_xticklabels([str(int(p)) for p in payload_len])
# ax.yaxis.set_major_formatter(ticker.FormatStrFormatter('%.2f'))

plt.grid(True, linestyle='--', alpha=0.6)
plt.legend(fontsize=10)
plt.tight_layout()

# 保存图像
out_path = os.path.join(output_dir, "throughput_vs_payload.png")
plt.savefig(out_path, dpi=300)
plt.close()

print(f"图已保存到 {out_path}")
