#!/bin/bash
# deploy.sh - 部署脚本

function print_usage {
    echo "用法: $0 [encrypt|decrypt|kms]"
    echo ""
    echo "  encrypt  - 部署加密XDP程序(在B机192.168.58.129上运行)"
    echo "  decrypt  - 部署解密XDP程序(在C机192.168.58.128上运行)"
    echo "  kms      - 部署KMS服务器(在A机192.168.58.132上运行)"
    echo ""
}

function setup_bpf_fs {
    # 检查BPF文件系统是否已挂载
    if ! mount | grep -q "bpf type bpf"; then
        echo "正在挂载BPF文件系统..."
        sudo mount -t bpf bpf /sys/fs/bpf/
    fi
    
    # 创建BPF对象目录
    sudo mkdir -p /sys/fs/bpf
    sudo chmod 755 /sys/fs/bpf
}

function deploy_encrypt {
    local interface="${1:-ens33}"
    
    echo "正在部署加密XDP程序到接口 $interface..."
    
    # 设置BPF文件系统
    setup_bpf_fs
    
    # 卸载之前的XDP程序
    sudo ip link set dev $interface xdp off 2>/dev/null || true
    
    # 加载XDP加密程序
    sudo ip link set dev $interface xdp obj encrypt.o sec xdp verbose
    
    # 显示XDP程序信息
    echo "XDP程序已加载到接口 $interface:"
    sudo bpftool prog show
    
    # 创建BPF Map目录
    sudo mkdir -p /sys/fs/bpf/sm4_key_encrypt
    
    # 检查是否已pin map
    if [ -f /sys/fs/bpf/sm4_key_encrypt/sm4_key_map ]; then
        echo "BPF Map已存在，无需再次pin"
    else
        # Pin BPF Map
        sudo bpftool map list
        MAP_ID=$(sudo bpftool map list | grep sm4_key_map | awk '{print $1}')
        if [ -n "$MAP_ID" ]; then
            sudo bpftool map pin id $MAP_ID /sys/fs/bpf/sm4_key_encrypt/sm4_key_map
            echo "BPF Map已pin到 /sys/fs/bpf/sm4_key_encrypt/sm4_key_map"
        else
            echo "错误: 无法找到sm4_key_map"
            return 1
        fi
    fi
    
    echo "加密XDP程序部署完成"
}

function deploy_decrypt {
    local interface="${1:-ens33}"
    
    echo "正在部署解密XDP程序到接口 $interface..."
    
    # 设置BPF文件系统
    setup_bpf_fs
    
    # 卸载之前的XDP程序
    sudo ip link set dev $interface xdp off 2>/dev/null || true
    
    # 加载XDP解密程序
    sudo ip link set dev $interface xdp obj decrypt.o sec xdp verbose
    
    # 显示XDP程序信息
    echo "XDP程序已加载到接口 $interface:"
    sudo bpftool prog show
    
    # 创建BPF Map目录
    sudo mkdir -p /sys/fs/bpf/sm4_key_decrypt
    
    # 检查是否已pin map
    if [ -f /sys/fs/bpf/sm4_key_decrypt/sm4_key_map ]; then
        echo "BPF Map已存在，无需再次pin"
    else
        # Pin BPF Map
        sudo bpftool map list
        MAP_ID=$(sudo bpftool map list | grep sm4_key_map | awk '{print $1}')
        if [ -n "$MAP_ID" ]; then
            sudo bpftool map pin id $MAP_ID /sys/fs/bpf/sm4_key_decrypt/sm4_key_map
            echo "BPF Map已pin到 /sys/fs/bpf/sm4_key_decrypt/sm4_key_map"
        else
            echo "错误: 无法找到sm4_key_map"
            return 1
        fi
    fi
    
    echo "解密XDP程序部署完成"
}

function deploy_kms {
    echo "正在部署KMS服务器..."
    
    # 杀掉之前的KMS进程
    pkill -f kms_server 2>/dev/null || true
    
    # 在后台启动KMS服务器
    ./kms_server &
    
    echo "KMS服务器已启动，监听端口9000"
}

# 主脚本开始
if [ $# -lt 1 ]; then
    print_usage
    exit 1
fi

case "$1" in
    encrypt)
        deploy_encrypt "$2"
        ;;
    decrypt)
        deploy_decrypt "$2"
        ;;
    kms)
        deploy_kms
        ;;
    *)
        echo "错误: 未知的部署类型: $1"
        print_usage
        exit 1
        ;;
esac
