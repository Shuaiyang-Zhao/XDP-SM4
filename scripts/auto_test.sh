#!/bin/bash
# auto_test.sh - 自动化测试脚本

echo "===== SM4 XDP 加解密系统自动化测试 ====="

# 检查环境
echo "检查系统环境..."
if ! ping -c 1 -W 1 192.168.58.129 > /dev/null 2>&1; then
    echo "❌ 无法连接到加密节点 (192.168.58.129)"
    echo "请确保加密节点已启动并XDP程序已加载"
    exit 1
fi

if ! ping -c 1 -W 1 192.168.58.128 > /dev/null 2>&1; then
    echo "❌ 无法连接到解密节点 (192.168.58.128)"
    echo "请确保解密节点已启动并XDP程序已加载"
    exit 1
fi

# 运行测试客户端
echo "运行测试客户端..."
./bin/test_client

# 分析测试结果
if [ $? -eq 0 ]; then
    echo "✅ 测试成功完成！"
else
    echo "❌ 测试失败！"
    exit 1
fi

echo "===== 测试完成 ====="