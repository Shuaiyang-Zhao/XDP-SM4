import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# 设置样式
sns.set(style="whitegrid")

# 读取 CSV 文件
df = pd.read_csv("udp_bandwidth_results.csv")

# 图 1：实际吞吐量 vs 期望带宽
plt.figure(figsize=(10, 6))
plt.plot(df["带宽(Mbps)"], df["发送吞吐量(Mbps)"], label="发送吞吐量", marker='o')
plt.plot(df["带宽(Mbps)"], df["接收吞吐量(Mbps)"], label="接收吞吐量", marker='s')
plt.xlabel("期望带宽 (Mbps)")
plt.ylabel("实际吞吐量 (Mbps)")
plt.title("实际吞吐量 vs 期望带宽")
plt.legend()
plt.tight_layout()
plt.savefig("throughput_vs_bandwidth.png")
plt.show()

# 图 2：丢包率 vs 期望带宽
plt.figure(figsize=(10, 6))
plt.plot(df["带宽(Mbps)"], df["丢包率(%)"], color='red', marker='^')
plt.xlabel("期望带宽 (Mbps)")
plt.ylabel("丢包率 (%)")
plt.title("丢包率 vs 期望带宽")
plt.tight_layout()
plt.savefig("packet_loss_vs_bandwidth.png")
plt.show()

# 图 3：发送/接收包数 vs 期望带宽
plt.figure(figsize=(10, 6))
plt.plot(df["带宽(Mbps)"], df["发送包数"], label="发送包数", marker='o')
plt.plot(df["带宽(Mbps)"], df["接收包数"], label="接收包数", marker='s')
plt.xlabel("期望带宽 (Mbps)")
plt.ylabel("包数")
plt.title("发送/接收包数 vs 期望带宽")
plt.legend()
plt.tight_layout()
plt.savefig("packets_vs_bandwidth.png")
plt.show()

# 图 4：抖动 vs 期望带宽
plt.figure(figsize=(10, 6))
plt.plot(df["带宽(Mbps)"], df["抖动(ms)"], color='green', marker='d')
plt.xlabel("期望带宽 (Mbps)")
plt.ylabel("抖动 (ms)")
plt.title("抖动 vs 期望带宽")
plt.tight_layout()
plt.savefig("jitter_vs_bandwidth.png")
plt.show()

