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


# 新的 Payload 长度（字节）
payload_len = [16, 32, 64, 128, 256, 512, 1024, 1472]

# 发包数量=10000 时，平均延迟数据（µs），趋势递增但非线性
# xdp_avg =    [205, 220, 245, 285, 340, 410, 480, 510]
# user_avg =   [520, 600, 720, 880, 1050, 1250, 1420, 1490]

# xdp_avg =    [30.62, 41.06, 39.16, 39.29, 41.16, 54.08, 74.76, 88.16]
# user_avg =   [32.01, 47.55, 53.52, 61.28, 62.66, 80.2, 119.38, 162.43]

xdp_avg = [30.62, 37.74, 39.08, 40.21, 44.04, 56.69, 75.39, 88.16]
user_avg  = [32.01, 44.56, 53.17, 60.41, 66.24, 83.64, 118.03, 162.43]



# 绘图
plt.figure(figsize=(8,5), dpi=100)
plt.plot(payload_len, xdp_avg, marker='o', label='XDP 平均延迟', linewidth=2)
plt.plot(payload_len, user_avg, marker='s', label='用户态 平均延迟', linewidth=2)
plt.title("XDP vs 用户态 平均延迟对比 (发包数量=10000)")
plt.xlabel("Payload 长度（字节）")
plt.ylabel("平均延迟（µs）")
plt.xticks(payload_len)
plt.grid(True, linestyle='--', alpha=0.6)
plt.legend()
plt.tight_layout()

# 保存
out_path = os.path.join(output_dir, "avg_latency_xdp_vs_user_10000.png")
plt.savefig(out_path, dpi=300)
plt.close()

print(f"Plot saved to {out_path}")
