#!/usr/bin/env bash
# 自动化吞吐量测试脚本（使用 iperf3 文本输出解析）
# Usage: ./test_throughput.sh <server_ip> [duration] [interval]

# 目标服务器 IP（B 机）
SERVER=${1:-192.168.58.128}
# 测试时长，单位秒（默认 30 秒）
DURATION=${2:-30}
# 输出间隔，单位秒（仅影响控制台输出，不影响测试）
INTERVAL=${3:-5}

# UDP payload 大小列表（字节），可根据需求调整
PAYLOAD_SIZES=(64 256 512 1024 1400)
# 测试速率列表（Mbps）
BANDWIDTHS=(200 400 600 800 1000 1200 1400 1600 1800 2000)

# 结果输出文件
OUTPUT_FILE="throughput_results.csv"

echo "payload_size,bandwidth_req_Mbps,transfer_MBytes,bandwidth_reported_Mbps,jitter_ms,loss_percent" > "$OUTPUT_FILE"

# 依赖检查
for cmd in iperf3 awk grep; do
  if ! command -v $cmd &>/dev/null; then
    echo "$cmd 未安装，请先安装 $cmd" >&2
    exit 1
  fi
done

# 测试循环
for bw in "${BANDWIDTHS[@]}"; do
  for size in "${PAYLOAD_SIZES[@]}"; do
    echo "Running: Bandwidth=${bw}Mbps, Payload=${size}B"
    # 运行 iperf3 文本模式
    OUT=$(iperf3 -c "$SERVER" -u -b "${bw}M" -l "$size" -t "$DURATION" -i "$INTERVAL" 2>&1)

    # 获取最后一行 receiver 汇总
    SUMMARY=$(echo "$OUT" | grep "receiver" | tail -n1)
    if [ -z "$SUMMARY" ]; then
      # 无效输出
      echo "$size,$bw,0.00,0.00,null,null" >> "$OUTPUT_FILE"
      continue
    fi

    # 解析字段
    # 总传输量
    TRANS_VAL=$(echo "$SUMMARY" | awk '{print $(NF-8)}')
    TRANS_UNIT=$(echo "$SUMMARY" | awk '{print $(NF-7)}')
    # 带宽值与单位
    BW_VAL=$(echo "$SUMMARY" | awk '{print $(NF-6)}')
    BW_UNIT=$(echo "$SUMMARY" | awk '{print $(NF-5)}')
    # 抖动
    JITTER=$(echo "$SUMMARY" | awk '{print $(NF-4)}')
    # 丢包率（带百分号）
    LOSS_PERCENT=$(echo "$SUMMARY" | awk '{print $(NF-1)}' | tr -d '()%')

    # 单位转换
    # Transfer to MBytes
    if [ "$TRANS_UNIT" == "GBytes" ]; then
      TRANS_MBYTES=$(awk "BEGIN{printf \"%.2f\", $TRANS_VAL*1024}")
    else
      TRANS_MBYTES=$TRANS_VAL
    fi
    # Bandwidth to Mbps
    if [[ "$BW_UNIT" == *"Gbits/sec" ]]; then
      BW_MBIT=$(awk "BEGIN{printf \"%.2f\", $BW_VAL*1000}")
    else
      BW_MBIT=$BW_VAL
    fi

    # 写入 CSV
    echo "$size,$bw,$TRANS_MBYTES,$BW_MBIT,$JITTER,$LOSS_PERCENT" >> "$OUTPUT_FILE"
  done
done

echo "Test finished. Results saved to $OUTPUT_FILE"

