#!/bin/bash
# build.sh - 编译所有组件

set -e  # 遇到错误立即退出

# 创建必要的目录
mkdir -p bin obj

echo "===== 编译KMS服务器 ====="
gcc -o bin/kms_server src/kms_server.c -lm

echo "===== 编译加密XDP程序 ====="
clang -O2 -g -Wall -target bpf -c src/encrypt.c -o obj/encrypt.o

echo "===== 编译解密XDP程序 ====="
clang -O2 -g -Wall -target bpf -c src/decrypt.c -o obj/decrypt.o

echo "===== 编译加密节点密钥客户端 ====="
gcc -o bin/encrypt_key_client src/encrypt_key_client.c -lbpf

echo "===== 编译解密节点密钥客户端 ====="
gcc -o bin/decrypt_key_client src/decrypt_key_client.c -lbpf

echo "===== 编译测试客户端 ====="
gcc -o bin/test_client src/test_client.c

echo "===== 编译完成 ====="
ls -l bin/ obj/

# 设置可执行权限
chmod +x bin/*
chmod +x scripts/*.sh