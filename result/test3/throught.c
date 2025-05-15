//gcc usr_sm4_no_return_with_stats.c sm4.c -o usr_sm4_no_return_with_stats

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include "sm4.h"

#define UDP_PORT 1233
#define MAX_BUFFER 2048
#define BLOCK_SIZE 16

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    unsigned char buffer[MAX_BUFFER];
    size_t total_bytes = 0;
    time_t start_time, end_time;

    // 创建 UDP 套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    // 配置本地地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("UDP Server is running on port %d...\n", UDP_PORT);

    // 使用固定密钥 (与 SM4 测试代码一致)
    u32 MK[4] = {0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210};
    u32 K[4];
    u32 RK[32];
    getRK(MK, K, RK);

    // 记录开始时间
    time(&start_time);

    while (1) {
        ssize_t recv_len = recvfrom(sockfd, buffer, MAX_BUFFER, 0,
                                    (struct sockaddr *)&client_addr, &client_addr_len);
        if (recv_len < 0) {
            perror("recvfrom error");
            continue;
        }

        // 更新统计信息
        total_bytes += recv_len;

        // 如果接收到的数据长度不是 16 字节的整数倍，则使用 0 填充
        size_t pad = (BLOCK_SIZE - (recv_len % BLOCK_SIZE)) % BLOCK_SIZE;
        size_t total_len = recv_len + pad;
        unsigned char *data = malloc(total_len);
        if (!data) {
            perror("malloc error");
            continue;
        }
        memcpy(data, buffer, recv_len);
        if (pad)
            memset(data + recv_len, 0, pad);

        unsigned char *encrypted = malloc(total_len);
        if (!encrypted) {
            free(data);
            perror("malloc error");
            continue;
        }

        // 分块加密（每块16字节）
        for (size_t i = 0; i < total_len; i += BLOCK_SIZE) {
            u32 X[4], Y[4];
            for (int j = 0; j < 4; j++) {
                u32 tmp;
                memcpy(&tmp, data + i + j * 4, 4);
                X[j] = ntohl(tmp);
            }
            encryptSM4(X, RK, Y);
            for (int j = 0; j < 4; j++) {
                u32 tmp = htonl(Y[j]);
                memcpy(encrypted + i + j * 4, &tmp, 4);
            }
        }

        free(data);
        free(encrypted);

        // 10秒统计一次吞吐率
        time(&end_time);
        if (difftime(end_time, start_time) >= 10) {
            double duration = difftime(end_time, start_time);
            double throughput_mbps = (total_bytes * 8) / (duration * 1000000.0);
            printf("\n[INFO] Duration: %.2f s, Total Bytes: %lu, Throughput: %.2f Mbps\n",
                   duration, total_bytes, throughput_mbps);
            total_bytes = 0;
            start_time = end_time;
        }
    }

    close(sockfd);
    return 0;
}
