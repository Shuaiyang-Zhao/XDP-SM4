#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <sys/resource.h>

#define DEST_IP "192.168.58.128"
#define DEST_PORT 1234
#define THREAD_NUM 1
#define MAX_PACKET_NUM 100
#define DATA_SIZE 15
#define LOCAL_PORT 1233

#define DEBUG

typedef struct {
    uint32_t seq;
    uint64_t timestamp;
    unsigned char send_data[DATA_SIZE];
} __attribute__((packed)) packet_t;

int sockfd;
struct sockaddr_in dest_addr;
struct sockaddr_in local_addr;

uint64_t get_timestamp_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)(ts.tv_sec * 1000000000ULL + ts.tv_nsec);
}

long get_cpu_time_microseconds() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_utime.tv_sec * 1000000L + usage.ru_utime.tv_usec +
           usage.ru_stime.tv_sec * 1000000L + usage.ru_stime.tv_usec;
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

void* sender_thread(void* arg) {
    uint32_t base_seq = (intptr_t)arg;
    packet_t pkt = {
        .send_data = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
                      0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32}
    };

    for (uint32_t i = 0; i < MAX_PACKET_NUM / THREAD_NUM; ++i) {
        pkt.seq = base_seq + i;
        pkt.timestamp = get_timestamp_ns();

        sendto(sockfd, &pkt, sizeof(pkt), 0,
               (struct sockaddr*)&dest_addr, sizeof(dest_addr));

#ifdef DEBUG
        print_packet(&pkt, "客户端发送");
#endif
    }
    return NULL;
}

int main() {
    long cpu_start = get_cpu_time_microseconds();

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("创建套接字失败");
        exit(EXIT_FAILURE);
    }

    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(LOCAL_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        perror("绑定失败");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DEST_PORT);
    inet_pton(AF_INET, DEST_IP, &dest_addr.sin_addr);

    pthread_t threads[THREAD_NUM];
    for (int i = 0; i < THREAD_NUM; ++i) {
        pthread_create(&threads[i], NULL, sender_thread, (void*)(intptr_t)(i * MAX_PACKET_NUM / THREAD_NUM));
    }

    for (int i = 0; i < THREAD_NUM; ++i) {
        pthread_join(threads[i], NULL);
    }

    long cpu_end = get_cpu_time_microseconds();
    long cpu_duration = cpu_end - cpu_start;

    printf("所有数据包已发送完成，客户端退出。\n");
    printf("客户端 CPU 使用时间：%.2f 毫秒\n", cpu_duration / 1000.0);

    close(sockfd);
    return 0;
}
