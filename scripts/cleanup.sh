#!/bin/bash
# cleanup.sh - 清理环境脚本

function print_usage {
    echo "用法: $0 [all|encrypt|decrypt|kms]"
    echo ""
    echo "  all      - 清理所有组件"
    echo "  encrypt  - 清理加密XDP程序(在B机192.168.58.129上运行)"
    echo "  decrypt  - 清理解密XDP程序(在C机192.168.58.128上运行)"
    echo "  kms      - 停止KMS服务器(在A机192.168.58.132上运行)"
    echo ""
}

function cleanup_encrypt {
    local interface="${1:-eth0}"
    
    echo "正在清理加密XDP程序..."
    
    # 卸载XDP程序
    sudo ip link set dev $interface xdp off 2>/dev/null || true
    
    # 卸载BPF Map
    if [ -f /sys/fs/bpf/sm4_key_encrypt/sm4_key_map ]; then
        sudo rm -f /sys/fs/bpf/sm4_key_encrypt/sm4_key_map
        echo "已移除BPF Map"
    fi
    
    # 杀掉密钥客户端进程
    pkill -f key_client 2>/dev/null || true
    
    echo "加密XDP程序清理完成"
}

function cleanup_decrypt {
    local interface="${1:-eth0}"
    
    echo "正在清理解密XDP程序..."
    
    # 卸载XDP程序
    sudo ip link set dev $interface xdp off 2>/dev/null || true
    
    # 卸载BPF Map
    if [ -f /sys/fs/bpf/sm4_key_decrypt/sm4_key_map ]; then
        sudo rm -f /sys/fs/bpf/sm4_key_decrypt/sm4_key_map
        echo "已移除BPF Map"
    fi
    
    # 杀掉密钥客户端进程
    pkill -f key_client 2>/dev/null || true
    
    echo "解密XDP程序清理完成"
}

function cleanup_kms {
    echo "正在停止KMS服务器..."
    
    # 杀掉KMS进程
    pkill -f kms_server 2>/dev/null || true
    
    echo "KMS服务器已停止"
}

# 主脚本开始
if [ $# -lt 1 ]; then
    print_usage
    exit 1
fi

case "$1" in
    all)
        cleanup_encrypt "$2"
        cleanup_decrypt "$2"
        cleanup_kms
        ;;
    encrypt)
        cleanup_encrypt "$2"
        ;;
    decrypt)
        cleanup_decrypt "$2"
        ;;
    kms)
        cleanup_kms
        ;;
    *)
        echo "错误: 未知的清理类型: $1"
        print_usage
        exit 1
        ;;
esac
