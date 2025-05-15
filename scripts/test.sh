#!/bin/bash
# test.sh - 测试脚本

# 创建测试消息
echo "This is a test message for SM4 encryption via XDP" > test/test_message.txt

echo "===== 测试流程 ====="
echo "1. 发送原始消息到加密节点..."
# 使用不同的源端口避免冲突
nc -u -w1 -p 1238 192.168.58.129 1234 < test/test_message.txt

echo "2. 监听并接收加密数据..."
nc -u -l 1235 > test/encrypted.dat

# 等待数据接收完成后按回车继续
read -p "加密数据已接收，按回车继续发送到解密节点..." 

echo "3. 发送加密数据到解密节点..."
nc -u -w1 -p 1239 192.168.58.128 1233 < test/encrypted.dat

echo "4. 监听并接收解密数据..."
nc -u -l 1236 > test/decrypted.dat

# 等待数据接收完成后按回车继续
read -p "解密数据已接收，按回车显示结果..." 

echo "===== 测试结果 ====="
echo "原始消息:"
cat test/test_message.txt
echo
echo "解密后消息:"
cat test/decrypted.dat
echo
echo "比较结果:"
diff -s test/test_message.txt test/decrypted.dat
