# XDP-SM4 加解密演示系统

本项目是基于 XDP（eXpress Data Path）的 SM4 加解密演示系统，可实现高性能的网络数据包加密和解密。


## 系统架构

系统部署在三台虚拟机上：

1. **A机** (192.168.58.132): 密钥管理服务器和测试客户端
2. **B机** (192.168.58.129): 部署加密 XDP 程序
3. **C机** (192.168.58.128): 部署解密 XDP 程序

工作流程：
- A机向B机发送UDP数据包，由B机的XDP程序进行加密并采用XDP_TX返回
- A机将加密后的数据发送到C机，由C机的XDP程序进行解密并采用XDP_TX返回
- A机接收解密结果，验证加解密流程的正确性
- 如果满足条件，也可以采用多台机器部署，加密服务器和解密服务器之间改为XDP_REDIRECT传递消息。

## 环境要求

- Linux 内核版本 5.0+ (推荐5.10+)
- LLVM/Clang 10.0+
- libbpf 开发库
- bpftool 工具
- GCC 编译器

## Flask API 端点

| 路由                    | 方法   | 功能描述                   |
| --------------------- | ---- | ---------------------- |
| `/`                   | GET  | 主页面，展示加解密 & 性能数据       |
| `/udp_encrypt`        | POST | 触发一次 UDP 加密测试          |
| `/udp_decrypt`        | POST | 触发一次 UDP 解密测试          |
| `/iperf_test`         | POST | 触发 iPerf3 网络带宽/时延测试（新） |
| `/query_remote_usage` | GET  | 抓取远端主机 CPU/内存使用数据      |

## 目录结构

```
├── .vscode/                        # VSCode 配置
├── docs/                           # 毕设文档
├── src/
│   ├── xdp/                        # XDP eBPF 程序
│   │   ├── encrypt.c
│   │   └── decrypt.c
│   ├── kms/                        # 密钥客户端与服务端
│   │   ├── encrypt_key_client.c
│   │   ├── decrypt_key_client.c
│   │   └── kms_server.c
│   └── flask_app/                  # Web 前后端（Flask）
│       ├── app.py
│       ├── udp_cs.py
│       ├── query_remote_usage.py
│       ├── system_performance.log
│       ├── templates/
│       │   └── index.html
│       └── static/
│           └── style.css
├── scripts/                        # 部署/构建/测试自动化脚本
│   ├── build.sh
│   ├── deploy.sh
│   ├── test.sh
│   ├── auto_test.sh
│   └── cleanup.sh
├── test/                           # 历史版本 & 单元/集成测试代码
│   ├── update1/                    # 负载部分只有数据 
│   ├── update2/                    # 负载部分（数据+辅助性能测量的数据） 
│   ├── test_udp/                   # 测试UDP通信
│   ├── test_en_de/                 # 测试加解密流程
│   └── test_performance/         
└── result/                         # 对应./test/test_performance/
    ├── a1/                     
    ├── a2/                      
    └── a3/                      

```


## 部署步骤

### 1. 准备工作

安装依赖项：

```bash
# Debian/Ubuntu 系统
sudo apt update
sudo apt install -y clang llvm libelf-dev gcc-multilib build-essential linux-tools-common linux-tools-generic libbpf-dev make
```

### 2. 获取源代码

https://github.com/Shuaiyang-Zhao/XDP-SM4   

 
### 3. 编译

根据机器角色编译相应组件：

#### A机 (KMS服务器和flask客户端):

```bash
cd XDP-SM4

# 编译 KMS 服务器
gcc -o src/kms_server src/kms/kms_server.c -lm

# 设置可执行权限
chmod +x src/kms_server 
```
<!-- 
或者直接使用编译脚本:(文档结构改变，部署脚本暂未更新)
```bash
cd XDP-SM4
./scripts/build.sh
``` -->

#### B机 (加密节点):

```bash
cd XDP-SM4

# 编译加密 XDP 程序
clang -O2 -g -Wall -target bpf -c src/xdp/encrypt.c -o src/xdp/encrypt.o

# 编译加密节点密钥客户端
gcc -o src/kms/encrypt_key_client src/kms/encrypt_key_client.c -lbpf

# 设置可执行权限
chmod +x src/kms/encrypt_key_client
```

<!-- 或者直接使用编译脚本:(文档结构改变，部署脚本暂未更新)
```bash
cd XDP-SM4
./scripts/build.sh
``` -->

#### C机 (解密节点):

```bash
cd XDP-SM4

# 编译解密 XDP 程序
clang -O2 -g -Wall -target bpf -c src/xdp/decrypt.c -o src/xdp/decrypt.o

# 编译解密节点密钥客户端
gcc -o src/kms/decrypt_key_client src/kms/decrypt_key_client.c -lbpf

# 设置可执行权限
chmod +x bin/decrypt_key_client
```

<!-- 或者直接使用编译脚本:(文档结构改变，部署脚本暂未更新)

```bash
cd XDP-SM4
./scripts/build.sh
``` -->

### 4. 部署和启动

按照以下顺序启动各个组件：

#### 第一步：启动 KMS 服务器 (A机)

```bash

./src/kms/kms_server -r （-r 随机生成密钥）
```

输出应类似于：

```
Using fixed test key (use -r for random keys)
Generated new key ID: 1
Key: 01234567 89abcdef fedcba98 76543210
...
KMS server started on port 9000
Waiting for clients...
```

此时 KMS 服务器将启动并在端口 9000 上监听，等待密钥客户端的连接。

或者使用部署脚本:（把deploy.sh放在kms_server同一目录）

```bash
cp ./scripts/deploy.sh ./src/kms/
sudo ./deploy.sh kms
```

#### 第二步：部署加密 XDP 程序 (B机)

```bash

# 确保 BPF 文件系统已挂载
sudo mount -t bpf bpf /sys/fs/bpf/

# 卸载之前的 XDP 程序（如果有）
sudo ip link set dev eth0 xdp off 2>/dev/null || true

# 加载加密 XDP 程序
sudo ip link set dev eth0 xdp obj obj/encrypt.o sec xdp verbose
```

或者使用部署脚本:（把deploy.sh放在encrypt.c同一目录）

```bash
cp ./scripts/deploy.sh ./src/xdp/
sudo ./deploy.sh encrypt eth0
```

输出应类似于：
```
正在部署加密XDP程序到接口 eth0...
正在挂载BPF文件系统...
XDP程序已加载到接口 eth0:
...
BPF Map已pin到 /sys/fs/bpf/sm4_key_encrypt/sm4_key_map
加密XDP程序部署完成
```

#### 第三步：启动加密节点的密钥客户端 (B机)

```bash
# 确认程序已加载
sudo bpftool prog show

# 找到 Map ID
MAP_ID=$(sudo bpftool map list | grep sm4_key_map | awk '{print $1}')
echo "Map ID: $MAP_ID"

# 创建目录并 Pin Map 到文件系统
sudo mkdir -p /sys/fs/bpf/sm4_key_encrypt
sudo bpftool map pin id $MAP_ID /sys/fs/bpf/sm4_key_encrypt/sm4_key_map

# 启动密钥客户端
./src/kms/encrypt_key_client 192.168.58.132 /sys/fs/bpf/sm4_key_encrypt/sm4_key_map 30
```

输出应类似于：
```
Starting SM4 key client
Server: 192.168.58.132:9000
BPF Map: /sys/fs/bpf/sm4_key_encrypt/sm4_key_map
Poll interval: 30 seconds

Requesting key from KMS server...
Received key:
Key ID: 1
Key: 0x01234567 0x89abcdef 0xfedcba98 0x76543210
...
This is a new key (previous ID: 0)
Successfully updated key in BPF map
```

#### 第四步：部署解密 XDP 程序 (C机)

```bash
cd XDP-SM4

# 确保 BPF 文件系统已挂载
sudo mount -t bpf bpf /sys/fs/bpf/

# 卸载之前的 XDP 程序（如果有）
sudo ip link set dev eth0 xdp off 2>/dev/null || true

# 加载解密 XDP 程序
sudo ip link set dev eth0 xdp obj obj/decrypt.o sec xdp verbose

# 确认程序已加载
sudo bpftool prog show
```

或者使用部署脚本:（把deploy.sh放在decrypt.c同一目录）

```bash
cp ./scripts/deploy.sh ./src/xdp/
sudo ./deploy.sh decrypt eth0

```

输出应类似于：

```bash

正在部署解密XDP程序到接口 eth0...
正在挂载BPF文件系统...
XDP程序已加载到接口 eth0:
...
BPF Map已pin到 /sys/fs/bpf/sm4_key_decrypt/sm4_key_map
解密XDP程序部署完成

```

#### 第五步：启动解密节点的密钥客户端  (C机)


```bash

cd XDP-SM4
# 找到 Map ID
MAP_ID=$(sudo bpftool map list | grep sm4_key_map | awk '{print $1}')
echo "Map ID: $MAP_ID"

# 创建目录并 Pin Map 到文件系统
sudo mkdir -p /sys/fs/bpf/sm4_key_decrypt
sudo bpftool map pin id $MAP_ID /sys/fs/bpf/sm4_key_decrypt/sm4_key_map

# 启动密钥客户端
./src/kms/decrypt_key_client 192.168.58.132 /sys/fs/bpf/sm4_key_decrypt/sm4_key_map 30
```

输出应类似于：
```
Starting SM4 key client
Server: 192.168.58.132:9000
BPF Map: /sys/fs/bpf/sm4_key_decrypt/sm4_key_map
Poll interval: 30 seconds

Requesting key from KMS server...
Received key:
Key ID: 1
Key: 0x01234567 0x89abcdef 0xfedcba98 0x76543210
...
This is a new key (previous ID: 0)
Successfully updated key in BPF map
```

#### 第四步：启动 Flask Web 平台
该flask_app需要在Pycharm（Professional Edition）中运行。学生优惠免费使用一年或者购买专业版。
   
<!-- ```bash
cd src/flask_app
FLASK_APP=app.py flask run --host=0.0.0.0 --port=5000
``` -->

<!-- ### 5. 测试系统

确保 A、B、C 三台机器上的所有组件都已启动后，在 A 机上运行测试客户端进行测试：

```bash
cd XDP-SM4
bash ./scripts/test.sh
```

测试客户端会自动执行以下操作：
1. 发送测试数据到 B 机进行加密
2. 接收 B 机返回的加密数据
3. 将加密数据发送到 C 机进行解密
4. 接收 C 机返回的解密数据
5. 显示测试结果

也可以使用自动化测试脚本：

```bash
cd XDP-SM4
bash ./scripts/auto_test.sh
```

### 6. 清理环境

测试完成后，可以使用清理脚本停止各个组件并卸载 XDP 程序：

```bash
# A机
cd XDP-SM4
./scripts/cleanup.sh kms

# B机
cd XDP-SM4
./scripts/cleanup.sh encrypt eth0

# C机
cd XDP-SM4
./scripts/cleanup.sh decrypt eth0
``` -->

## 注意事项

1. **网络接口名称**：部署脚本中使用的是 `eth0`，请根据系统实际情况修改为相应的网络接口名称。

2. **IP 地址**：代码中使用的 IP 地址为：
   - A机: 192.168.58.132
   - B机: 192.168.58.129
   - C机: 192.168.58.128
   
   如果您的环境不同，请相应地修改源代码和脚本中的 IP 地址。

3. **端口使用**：
   - KMS 服务器：9000
   - 加密服务：1234 (接收)，1235 (发送回复)
   - 解密服务：1233 (接收)，1236 (发送回复)
   
   请确保这些端口在系统中未被占用。

4. **权限**：运行 XDP 程序和访问 BPF Map 需要 root 权限，请确保使用 sudo 运行相关命令。

5. **密钥一致性**：系统正常工作的前提是加密和解密节点使用相同的密钥。如果出现加密后无法正确解密的情况，请检查两个节点的密钥是否一致。

## 调试技巧

1. **查看 BPF 输出**：
   ```bash
   sudo cat /sys/kernel/debug/tracing/trace_pipe | grep bpf
   ```

2. **检查 BPF Map 状态**：
   ```bash
   sudo bpftool map dump name sm4_key_map
   # 或
   sudo bpftool map dump pinned /sys/fs/bpf/sm4_key_encrypt/sm4_key_map
   ```

3. **查看网络接口 XDP 状态**：
   ```bash
   sudo bpftool net
   # 或
   sudo ip link show dev eth0
   ```

4. **重新挂载 BPF 文件系统**：
   ```bash
   sudo umount /sys/fs/bpf
   sudo mount -t bpf bpf /sys/fs/bpf
   ```

## 常见问题解决

1. **"Failed to open BPF map" 错误**：
   - 确保 BPF 文件系统已挂载：`sudo mount -t bpf bpf /sys/fs/bpf/`
   - 确保路径存在：`sudo mkdir -p /sys/fs/bpf/sm4_key_encrypt`
   - 确认权限：`sudo chmod 755 /sys/fs/bpf/sm4_key_encrypt`

2. **"bind failed: Address already in use" 错误**：
   - 使用 `sudo netstat -tulpn | grep <端口号>` 确认端口是否被占用
   - 修改测试客户端源码中的端口号
   - 或终止占用端口的进程：`sudo kill <PID>`

3. **加密正常但解密结果不正确**：
   - 检查加密和解密节点的密钥是否一致
   - 查看内核日志获取更多信息：`dmesg | grep bpf`
   - 确保填充和解析逻辑正确处理数据长度

4. **XDP 程序加载失败**：
   - 检查内核版本：`uname -r`，确保内核支持 XDP
   - 检查网卡驱动是否支持 XDP
   - 尝试使用 SKB 模式：`ip link set dev eth0 xdp-skb obj obj/encrypt.o sec xdp`

## 联系 & 贡献

* 作者：赵帅阳
* 邮箱：[shuaiyang.zhao@foxmail.com](mailto:your.email@example.com)
* 欢迎通过 Issues 或 Pull Requests 提交建议与改进！



