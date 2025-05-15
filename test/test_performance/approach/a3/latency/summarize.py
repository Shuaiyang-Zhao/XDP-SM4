import csv
import os
import re
import statistics

# 要扫描的目录
directory = "."

# 存储统计结果
results = []

# 遍历当前目录下所有 .csv 文件
for filename in os.listdir(directory):
    if filename.endswith(".csv") and filename.startswith("latency_"):
        # 解析文件名中的负载长度和发包数量
        match = re.match(r"latency_(\d+)_(\d+)\.csv", filename)
        if not match:
            continue
        payload_len = int(match.group(1))
        pkt_count = int(match.group(2))

        delays = []

        # 读取时延列（第三列）
        with open(os.path.join(directory, filename), 'r') as f:
            reader = csv.reader(f)
            for row in reader:
                try:
                    delay = int(row[2])
                    delays.append(delay)
                except (IndexError, ValueError):
                    continue

        if delays:
            min_delay = min(delays)
            avg_delay = round(statistics.mean(delays), 2)
            max_delay = max(delays)
            results.append((payload_len, pkt_count, min_delay, avg_delay, max_delay))

# 排序：先按负载长度，再按发包数量
results.sort(key=lambda x: (x[0], x[1]))

# 写入汇总 CSV 文件
with open("latency_summary.csv", "w", newline='') as f:
    writer = csv.writer(f)
    writer.writerow(["负载长度(字节)", "发包数量", "最小时延(μs)", "平均时延(μs)", "最大时延(μs)"])
    writer.writerows(results)

print("✅ 汇总完成，结果已保存至 latency_summary.csv")
