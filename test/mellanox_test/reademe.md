要使用 `iperf` 测量 A 机器到 B 机器的吞吐率，可以按以下步骤操作：

1. **在 B 机器上启动 `iperf` 服务器**：
   B 机器（IP 地址 192.168.58.128）上启动 `iperf` 服务器来接收 UDP 数据包：

   ```bash
   iperf3 -s -p 1233
   ```

2. **在 A 机器上启动 `iperf` 客户端**：
   A 机器（IP 地址 192.168.58.129）上启动 `iperf` 客户端，发送 UDP 数据包到 B 机器（192.168.58.128:1233）：

   ```bash
   iperf3 -c 192.168.58.128 -p 1233 -u -b 0 -t 60 -i 1 -l 1024
   ```

   参数解释：
   - `-c 192.168.58.128`：指定 B 机器的 IP 地址作为目标。
   - `-p 1233`：指定 B 机器的端口号。
   - `-u`：使用 UDP 协议进行测试。
   - `-b 0`：指定带宽为 0，表示不限制带宽，使用最佳吞吐率。
   - `-t 60`：指定测试持续时间为 60 秒。
   - `-i 1`：每秒输出一次吞吐率信息。
   - `-l 1024`：设置每个 UDP 数据包的负载为 1024 字节。

这样就可以测量 A 机器向 B 机器发送 UDP 数据包的吞吐率了。如果需要记录更详细的信息，可以将输出结果重定向到文件进行分析：

```bash
iperf3 -c 192.168.58.128 -p 1233 -u -b 0 -t 60 -i 1 > iperf_output.txt
iperf3 -c 192.168.58.128 -p 1233 -u -b 0 -t 60 -i 1 -l 16 >> iperf_output2.txt
```
```
1) 打开转发（可选，只要用了 REDIRECT 就不一定非要）
sudo sysctl -w net.ipv4.ip_forward=1

2) 只做 TCP 重定向，让控制通道通
sudo iptables -t nat -A PREROUTING -p tcp --dport 1240 -j REDIRECT --to-ports 1239

3) 启动 iperf3 服务端
iperf3 -s -p 1239 &

4) 启动你的加密代理
./udp_sm4_proxy

iperf3 -c 192.168.58.128 -p 1240 -u -b 0 -t 60 -i 10 -l 1472 >> iperf_output3.txt


```

下面是一个 `netperf` 测量吞吐量的 README.md 文件示例，适用于你使用 `netperf` 来测试网络吞吐量的情况。

```
# 使用 netperf 测量网络吞吐量

本指南展示了如何使用 `netperf` 工具来测量网络吞吐量，适用于 UDP 协议。假设你已经有两台机器 A 和 B，并且它们通过 UDP 进行通信。

## 环境设置

- **A 机器**：发送端，IP 地址：`192.168.58.129`，端口：`1235`。
- **B 机器**：接收端，IP 地址：`192.168.58.128`，端口：`1233`。
- **netperf**：网络性能测试工具。

## 安装 netperf

在 Ubuntu 或 Debian 系列的操作系统上，可以使用以下命令安装 `netperf`：

```bash
sudo apt-get update
sudo apt-get install netperf
```

## 启动 netserver（在 B 机器上）

首先，在接收端 **B 机器** 上启动 `netserver`。`netserver` 是 `netperf` 的配套服务器端工具，用于接收来自客户端的连接。

在 **B 机器** 上运行以下命令：

```bash
netserver -p 1233
```

这会启动 `netserver` 并使其监听端口 `1233`。

## 在 A 机器上进行吞吐量测试

接下来，在 **A 机器** 上使用 `netperf` 进行吞吐量测试。我们将使用 TCP 或 UDP 吞吐量测试（`TCP_STREAM` 或 `UDP_STREAM`），并指定 **B 机器** 的 IP 地址和端口。

### 测试 UDP 吞吐量

如果你想测量 UDP 吞吐量，运行以下命令：

```bash
netperf -H 192.168.58.128 -t UDP_STREAM -p 1233
```

`netperf` 会发送数据包并报告吞吐量（单位：Mbps），包括数据包发送和接收的总速率。

### 测试时可以指定测试时间

例如，若想测试 30 秒的吞吐量，可以使用 `-l` 参数指定时间（单位：秒）：

```bash
netperf -H 192.168.58.128 -t UDP_STREAM -p 1233 -l 30
```

此命令将测量从 A 机器到 B 机器的 UDP 吞吐量，持续 30 秒。

## 结果解析

`netperf` 的输出将包含以下字段：

```
MSS size 1460, MTU size 1500, and buffer size 256000
 # Running Test
 UDP_STREAM Test
 Recv   Send   Total
 0.0000 1.0000 1.0000
```

- **Recv**：接收的吞吐量，单位为 Mbps。
- **Send**：发送的吞吐量，单位为 Mbps。
- **Total**：总吞吐量，单位为 Mbps。

### 吞吐量单位

- `Mbps`：兆比特每秒。
- `Kbps`：千比特每秒。

## 高级选项

### 设置负载大小

你可以使用 `-s` 参数指定负载大小：

```bash
netperf -H 192.168.58.128 -t UDP_STREAM -p 1233 -l 60 -i 1 -o 16 >> iperf_output.txt

```

这个命令会将每个数据包的负载大小设置为 1024 字节。

### 设置测试报文大小

你还可以通过 `-S` 参数设置报文大小（默认的 1500 字节）：

```bash
netperf -H 192.168.58.128 -t UDP_STREAM -p 1233 -S 1400
```


要统计 **CPU 使用情况** 和 **内存使用情况**，可以使用以下两种方法：

### 1. **使用 `top` 或 `htop` 与 `watch` 配合**
`top` 或 `htop` 可以直接显示 CPU 和内存的使用情况，并且通过 `watch` 命令定期输出。这是最简单、最常见的方式。

- **`top`**：默认显示 CPU 和内存使用情况。
- **`htop`**：提供更友好的界面，并显示更多的信息。

#### 示例：
1. **`top` 配合 `watch` 命令**：
    ```bash
    watch -n 1 "top -n 1 -b >> top_output.log"
    ```
    这条命令每秒执行一次 `top` 命令，并将输出追加到 `top_output.log` 文件中。`-n 1` 表示只执行一次，`-b` 是批处理模式，不显示界面。

2. **`htop` 配合 `watch` 命令**：
    ```bash
    watch -n 1 "htop -b -n 1 >> htop_output.log"
    ```
    这里 `-b` 表示批处理模式，`-n 1` 表示运行一次，输出将记录到 `htop_output.log` 文件中。

### 2. **使用 `mpstat` 和 `free` 来分别统计 CPU 和内存**
如果你只关心 CPU 和内存的具体数值而不需要像 `top` 那样的实时交互式界面，可以分别使用 `mpstat` 和 `free` 命令来监控 CPU 和内存：

1. **使用 `mpstat` 监控 CPU 使用情况**：
    `mpstat` 是一个用来监控 CPU 使用的工具，以下命令每秒显示一次 CPU 使用情况，并将其保存到文件中：
    ```bash
    watch -n 1 "mpstat 1 >> cpu_usage.log"
    ```
    这里 `mpstat 1` 每秒显示一次 CPU 的使用情况，`-n 1` 是每秒执行一次，输出保存到 `cpu_usage.log`。

2. **使用 `free` 监控内存使用情况**：
    `free` 是用来显示系统内存的工具，以下命令每秒获取一次内存使用情况，并保存到文件中：
    ```bash
    watch -n 1 "free -h >> memory_usage.log"
    ```
    `-h` 参数会以人类可读的格式（比如 MB、GB）显示内存使用情况，输出保存到 `memory_usage.log`。

### 3. **使用 `btop` 手动记录 CPU 和内存使用情况**
如果你更喜欢 `btop` 的界面，可以在 `btop` 中查看实时的 CPU 和内存使用情况，然后手动记录。虽然 `btop` 本身不支持自动保存日志，但你可以通过调整刷新频率来增加可见性并手动截屏或记录数据。

### 总结：
- **`top`** 或 **`htop`** 配合 `watch` 可以实时监控并记录 CPU 和内存使用情况，适合快速获取资源使用情况。
- **`mpstat`** 用于专门监控 CPU 使用，**`free`** 用于监控内存使用。
- **`btop`** 适合实时观察并手动记录，但不支持自动保存日志。

推荐使用 **`top` 或 `htop` 配合 `watch`** 来同时记录 CPU 和内存使用情况，这种方法最为常见且易于操作。