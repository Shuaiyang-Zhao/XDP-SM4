#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>

#define PORT_ENCRYPT 1233
#define LOCAL_PORT_ENCRYPT 1235
#define ENCRYPT_IP "192.168.58.128"
#define TEST_ROUNDS 10

unsigned char MESSAGE[] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32
};

double time_diff_us(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec);
}

int bind_socket(int port) {
    int sockfd;
    struct sockaddr_in addr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("创建 socket 失败");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("绑定本地端口失败");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

int main() {
    int sock_encrypt;
    struct sockaddr_in b_addr;
    unsigned char buf[2048];
    ssize_t n;
    size_t plain_len = sizeof(MESSAGE);

    sock_encrypt = bind_socket(LOCAL_PORT_ENCRYPT);

    memset(&b_addr, 0, sizeof(b_addr));
    b_addr.sin_family = AF_INET;
    b_addr.sin_port = htons(PORT_ENCRYPT);
    inet_pton(AF_INET, ENCRYPT_IP, &b_addr.sin_addr);

    double delays[TEST_ROUNDS];
    int total_sent = 0, total_recv = 0;
    double total_throughput_mbps = 0;

    for (int i = 0; i < TEST_ROUNDS; i++) {
        struct timeval start, end;
        gettimeofday(&start, NULL);
        sendto(sock_encrypt, MESSAGE, plain_len, 0, (struct sockaddr*)&b_addr, sizeof(b_addr));
        total_sent++;

        time_t wait_start = time(NULL);
        int success = 0;
        while (1) {
            n = recvfrom(sock_encrypt, buf, sizeof(buf), 0, NULL, NULL);
            if (n > 0) {
                gettimeofday(&end, NULL);
                success = 1;
                break;
            }
            if (time(NULL) - wait_start > 3) break;
            usleep(100 * 1000);
        }

        if (!success) {
            fprintf(stderr, "❌ 第 %d 轮未收到加密响应\n", i + 1);
            continue;
        }

        double delay_us = time_diff_us(start, end);
        double throughput_mbps = (plain_len * 8.0) / delay_us;  // bits / us = Mbps

        delays[total_recv] = delay_us;
        total_throughput_mbps += throughput_mbps;
        total_recv++;
    }

    // 汇总统计
    double min_delay = 1e9, max_delay = 0, sum_delay = 0;
    for (int i = 0; i < total_recv; i++) {
        if (delays[i] < min_delay) min_delay = delays[i];
        if (delays[i] > max_delay) max_delay = delays[i];
        sum_delay += delays[i];
    }

    double avg_delay = total_recv ? (sum_delay / total_recv) : 0;
    double avg_throughput = total_recv ? (total_throughput_mbps / total_recv) : 0;

    printf("\n📊 加密测试结果 (%d/%d 成功轮次):\n", total_recv, TEST_ROUNDS);
    printf("🚀 平均系统吞吐率：%.3f Mbps\n", avg_throughput);
    printf("⏱️ 加密处理时间：最小 %.0f us, 平均 %.0f us, 最大 %.0f us\n", min_delay, avg_delay, max_delay);
    printf("📉 包丢失率：%.2f %%\n", 100.0 * (total_sent - total_recv) / total_sent);

    close(sock_encrypt);
    return 0;
}

