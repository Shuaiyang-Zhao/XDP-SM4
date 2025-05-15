#!/bin/bash

# 清空旧日志
> metrics.log

# 启动 Python 动态绘图程序（后台）
python3 plot_dynamic.py &

# 启动接收端程序（写入日志）
./server > metrics.log

# start_test.sh

#!/bin/bash
