#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <math.h>
#include <sys/socket.h>

#define LISTEN_IP "192.168.58.129"
#define LISTEN_PORT 123
#define MAX_PACKET_NUM 10
#define LOSS_THRESHOLD_NS 50000000  // 丢包阈值 (50ms)
#define PERCENTILE_95 0.95
#define PERCENTILE_99 0.99

// 数据包结构定义
typedef struct {
    uint64_t timestamp;
    char data[64];  // 数据部分
} packet_t;

// 接收统计数据结构
typedef struct {
    uint64_t total_packets;
    uint64_t lost_packets;
    uint64_t total_latency_us;
    double min_latency_us;
    double max_latency_us;
    double sum_squared_latency;
    double *latencies_us;
    size_t latency_count;
    double p95_latency_us;
    double p99_latency_us;
    double jitter_us;
    double average_latency_us;
    double std_deviation_us;
    double loss_rate;
} recv_stats_t;

// 获取当前时间戳（单位：纳秒）
uint64_t get_timestamp_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// 比较函数，用于 qsort 排序
int compare(const void* a, const void* b) {
    double diff = *(double*)a - *(double*)b;
    return (diff > 0) - (diff < 0);
}

// 更新接收端统计数据
void update_recv_stats(recv_stats_t* stats, uint64_t latency_us) {
    stats->total_packets++;

    // 丢包判断
    if (latency_us > LOSS_THRESHOLD_NS / 1000) {  // 丢包阈值（转换为微秒）
        stats->lost_packets++;
    } else {
        // 更新延迟统计
        stats->total_latency_us += latency_us;
        if (latency_us < stats->min_latency_us) stats->min_latency_us = latency_us;
        if (latency_us > stats->max_latency_us) stats->max_latency_us = latency_us;

        stats->latencies_us[stats->latency_count++] = latency_us;
        stats->sum_squared_latency += latency_us * latency_us;

        // 计算 P95 和 P99 延迟
        // 对延迟数组进行排序
        if (stats->latency_count >= 2) {
            // 排序
            qsort(stats->latencies_us, stats->latency_count, sizeof(double), compare);
            stats->p95_latency_us = stats->latencies_us[(int)(stats->latency_count * PERCENTILE_95)];
            stats->p99_latency_us = stats->latencies_us[(int)(stats->latency_count * PERCENTILE_99)];
        }
    }

    // 计算丢包率
    stats->loss_rate = (double)stats->lost_packets / stats->total_packets;

    // 计算平均延迟
    stats->average_latency_us = stats->total_latency_us / stats->latency_count;

    // 计算标准差
    double mean_square = stats->sum_squared_latency / stats->latency_count;
    stats->std_deviation_us = sqrt(mean_square - stats->average_latency_us * stats->average_latency_us);

    // 计算抖动 (Jitter)
    if (stats->latency_count > 1) {
        stats->jitter_us = stats->latencies_us[stats->latency_count - 1] - stats->latencies_us[stats->latency_count - 2];
    }
}

// 输出接收端统计数据
void print_recv_stats(const recv_stats_t* stats) {
    // 清屏
    printf("\033[H\033[J");

    // 打印统计信息
    printf("\n丢包率: %.2f%%\n", stats->loss_rate * 100);
    printf("平均延迟: %.2f 微秒\n", stats->average_latency_us);
    printf("最小延迟: %.2f 微秒\n", stats->min_latency_us);
    printf("最大延迟: %.2f 微秒\n", stats->max_latency_us);
    printf("P95 延迟: %.2f 微秒\n", stats->p95_latency_us);
    printf("P99 延迟: %.2f 微秒\n", stats->p99_latency_us);
    printf("标准差: %.2f 微秒\n", stats->std_deviation_us);
    printf("抖动: %.2f 微秒\n", stats->jitter_us);
}

// 接收端主程序
int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len = sizeof(cliaddr);
    packet_t pkt;

    // 初始化统计信息
    recv_stats_t stats = {0};
    stats.latencies_us = malloc(MAX_PACKET_NUM * sizeof(double));  // 分配延迟数组

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
    uint64_t last_print_time = start_time;

    printf("服务端已启动，监听地址 %s:%d ...\n", LISTEN_IP, LISTEN_PORT);

    // 单线程接收数据包
    while (count < MAX_PACKET_NUM) {
        ssize_t n = recvfrom(sockfd, &pkt, sizeof(pkt), 0,
                             (struct sockaddr*)&cliaddr, &len);
        if (n < 0) {
            perror("接收失败");
            continue;
        }

        uint64_t now = get_timestamp_ns();
        uint64_t rtt = now >= pkt.timestamp ? (now - pkt.timestamp) : 0;

        // 更新统计信息
        update_recv_stats(&stats, rtt / 1000);  // 转换为微秒

        // 每秒刷新一次统计信息
        uint64_t elapsed_time = now - last_print_time;
        if (elapsed_time >= 1000000000ULL) {  // 每秒刷新
            print_recv_stats(&stats);
            last_print_time = now;
        }

        count++;
    }

    uint64_t end_time = get_timestamp_ns();
    double duration_sec = (end_time - start_time) / 1e9;

    printf("\n======= 测试总结 =======\n");
    printf("总接收数据包数  ：%u 个\n", count);
    printf("延迟超 50ms 的丢包：%u 个\n", loss_count);
    printf("丢包率           ：%.2f%%\n", 100.0 * loss_count / count);
    printf("平均延迟         ：%.2f 毫秒\n", total_latency / count / 1e6);
    printf("最小延迟         ：%.2f 毫秒\n", min_latency / 1e6);
    printf("最大延迟         ：%.2f 毫秒\n", max_latency / 1e6);
    printf("测试总时长       ：%.2f 秒\n", duration_sec);

    free(stats.latencies_us);  // 释放延迟数组
    close(sockfd);
    return 0;
}
