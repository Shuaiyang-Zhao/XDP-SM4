#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <pthread.h>

#define LISTEN_IP "192.168.58.129"
#define LISTEN_PORT 1233
#define MAX_PACKET_NUM 10
#define DATA_SIZE 15
#define LOSS_THRESHOLD_NS 50000000  // 50ms

#define DEBUG

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

#ifdef DEBUG
void print_packet(packet_t* pkt, const char* prefix) {
    printf("[%s] 序号: %u, 时间戳: %" PRIu64 ", 数据: ", prefix, pkt->seq, pkt->timestamp);
    for (int i = 0; i < DATA_SIZE; ++i) {
        printf("%02x ", pkt->send_data[i]);
    }
    printf("\n");
}
#endif

// 单独处理每个接收到的数据包的线程
void* handle_packet(void* arg) {
    packet_t pkt = *((packet_t*)arg);
    free(arg);

    uint64_t now = get_timestamp_ns();
    uint64_t rtt = now >= pkt.timestamp ? (now - pkt.timestamp) : 0;

    // 打印接收到的数据包内容
    print_packet(&pkt, "接收");

    // 如果延迟大于阈值，丢包
    if (rtt > LOSS_THRESHOLD_NS) {
        // 丢包处理逻辑（如果需要）
    }

    // 打印延迟信息
    printf("已收序号: %u，接收时间戳：%" PRIu64 ", 延迟: %.2f ms\n", pkt.seq, now, rtt / 1e6);

    return NULL;
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

    printf("服务端已启动，监听地址 %s:%d ...\n", LISTEN_IP, LISTEN_PORT);

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

        // 为每个包创建一个新线程来处理
        packet_t* pkt_copy = malloc(sizeof(packet_t));
        if (pkt_copy != NULL) {
            memcpy(pkt_copy, &pkt, sizeof(packet_t));
            pthread_t thread;
            pthread_create(&thread, NULL, handle_packet, pkt_copy);
            pthread_detach(thread);  // 线程分离，防止主线程等待
        }

        ++count;
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

    close(sockfd);
    return 0;
}
