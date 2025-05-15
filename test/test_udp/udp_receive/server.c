#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <math.h>

#define PACKET_SIZE (sizeof(packet_t))  // 数据包大小，单位字节

#define LISTEN_IP "192.168.58.129"
#define LISTEN_PORT 1233
#define MAX_PACKET_NUM 1000000
#define DATA_SIZE 1450
#define LOSS_THRESHOLD_NS 500000000  // 50ms

#define IS_DEBUG 0

typedef struct {
    uint32_t seq;
    uint64_t timestamp;
    unsigned char send_data[DATA_SIZE];
} __attribute__((packed)) packet_t;

uint64_t get_timestamp_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts); // 保持一致
    return (uint64_t)(ts.tv_sec * 1000000000ULL + ts.tv_nsec);
}

void print_packet(packet_t* pkt, const char* prefix) {
    printf("[%s] 序号: %u, 时间戳: %" PRIu64 ", 数据: ", prefix, pkt->seq, pkt->timestamp);
    for (int i = 0; i < DATA_SIZE; ++i) {
        printf("%02x ", pkt->send_data[i]);
    }
    printf("\n");
}

// 处理每个接收到的数据包的函数
void handle_packet(packet_t* pkt) {
    uint64_t now = get_timestamp_ns();
    uint64_t rtt = now >= pkt->timestamp ? (now - pkt->timestamp) : 0;
   
    // 如果延迟大于阈值，丢包
    if (rtt > LOSS_THRESHOLD_NS) {
        // 丢包处理逻辑（如果需要）
    }

    // 打印延迟信息
    if (IS_DEBUG) {
        print_packet(pkt, "接收");
        printf("已收序号: %u，接收时间戳：%" PRIu64 ", 延迟: %.2f ms\n", pkt->seq, now, rtt / 1e6);
    }
}

int compare(const void* a, const void* b) {
    return (*(double*)a - *(double*)b);
}

void print_test_summary(int total_packets, int lost_packets, double avg_delay, double min_delay, double max_delay, double p95_delay, double p99_delay, double stddev, double jitter, double duration, double throughput) {
    printf("\033[H\033[J");  // 清屏
    printf("======= 测试总结 =======\n");
    printf("吞吐率           ：%.2f MBps \n", throughput);  // 将吞吐率放在前面
    printf("总接收数据包数  ：%d 个\n", total_packets);
    printf("延迟超 50ms 的丢包：%d 个\n", lost_packets);
    printf("丢包率           ：%.2f%%\n", (double)lost_packets / total_packets * 100);
    printf("平均延迟         ：%.2f 微秒\n", avg_delay);
    printf("最小延迟         ：%.2f 微秒\n", min_delay);
    printf("最大延迟         ：%.2f 微秒\n", max_delay);
    printf("P95 延迟         ：%.2f 微秒\n", p95_delay);
    printf("P99 延迟         ：%.2f 微秒\n", p99_delay);
    printf("标准差           ：%.2f 微秒\n", stddev);
    printf("抖动             ：%.2f 微秒\n", jitter);
    printf("测试总时长       ：%.2f 秒\n", duration);

    fflush(stdout);  // 强制刷新输出
    FILE *fp = fopen("./realtime_stats.csv", "a");
    if (fp) {
        fprintf(fp, "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n", 
            duration, avg_delay, min_delay, max_delay, p95_delay, p99_delay, stddev, jitter, throughput);
        fclose(fp);
    }
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len = sizeof(cliaddr);
    packet_t pkt;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("创建套接字失败");
        exit(EXIT_FAILURE);
    }

    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(LISTEN_PORT);
    inet_pton(AF_INET, LISTEN_IP, &servaddr.sin_addr);

    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("绑定失败");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    uint32_t count = 0, loss_count = 0;
    uint64_t total_latency = 0, min_latency = UINT64_MAX, max_latency = 0;
    uint64_t start_time = get_timestamp_ns();

    // 为延迟数据分配内存
    double latencies_us[MAX_PACKET_NUM];
    size_t latency_count = 0;
    double sum_squared_latency = 0;

    printf("服务端已启动，监听地址 %s:%d ...\n", LISTEN_IP, LISTEN_PORT);

    // 时间记录用于每秒刷新输出
    uint64_t last_output_time = get_timestamp_ns();

    while (count < MAX_PACKET_NUM) {
        ssize_t n = recvfrom(sockfd, &pkt, sizeof(pkt), 0,
                             (struct sockaddr*)&cliaddr, &len);
        if (n < 0) {
            perror("接收失败");
            continue;
        }

        uint64_t now = get_timestamp_ns();
        uint64_t rtt = now >= pkt.timestamp ? (now - pkt.timestamp) : 0;

        if (rtt > LOSS_THRESHOLD_NS) {
            loss_count++;
        }

        total_latency += rtt;
        if (rtt < min_latency) min_latency = rtt;
        if (rtt > max_latency) max_latency = rtt;

        // 记录延迟数据
        latencies_us[latency_count] = rtt / 1000.0;  // 转换为微秒
        sum_squared_latency += latencies_us[latency_count] * latencies_us[latency_count];
        latency_count++;

        // 直接处理任务
        handle_packet(&pkt);

        ++count;

        // 每秒刷新一次输出
        uint64_t current_time = get_timestamp_ns();
        if (current_time - last_output_time >= 1000000000ULL) {  
            last_output_time = current_time;

            uint64_t end_time = get_timestamp_ns();
            double duration_sec = (end_time - start_time) / 1e9;

            // 排序延迟数据用于计算 P95 和 P99
            qsort(latencies_us, latency_count, sizeof(double), compare);

            double p95_latency_us = latencies_us[(int)(latency_count * 0.95)];
            double p99_latency_us = latencies_us[(int)(latency_count * 0.99)];
            double avg_latency_us = total_latency / (double)count / 1000.0;
            double stddev_latency_us = sqrt((sum_squared_latency / latency_count) - (avg_latency_us * avg_latency_us));
            double jitter_us = max_latency - min_latency;

            // double throughput = count / duration_sec;  // 每秒包数
            // 计算吞吐率为 MBps
            double throughput = (count * PACKET_SIZE) / (duration_sec * 1024 * 1024);  // 单位：MBps

            print_test_summary(count, loss_count, avg_latency_us, min_latency / 1000.0, max_latency / 1000.0, p95_latency_us, p99_latency_us, stddev_latency_us, jitter_us, duration_sec, throughput);
        }
    }

    close(sockfd);
    return 0;
}
