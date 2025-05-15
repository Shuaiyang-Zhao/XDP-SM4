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
    time_t start_time = time(NULL);

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

    // 主循环
    while (1) {
        ssize_t recv_len = recvfrom(sockfd, buffer, MAX_BUFFER, 0,
                                    (struct sockaddr *)&client_addr, &client_addr_len);
        if (recv_len < 0) {
            perror("recvfrom error");
            continue;
        }

        // 统计接收到的数据量
        total_bytes += recv_len;

        // 分块加密（每块16字节）
        for (size_t i = 0; i < recv_len; i += BLOCK_SIZE) {
            u32 X[4], Y[4];
            for (int j = 0; j < 4; j++) {
                u32 tmp;
                memcpy(&tmp, buffer + i + j * 4, 4);
                X[j] = ntohl(tmp);
            }
            encryptSM4(X, RK, Y);
            for (int j = 0; j < 4; j++) {
                u32 tmp = htonl(Y[j]);
                memcpy(buffer + i + j * 4, &tmp, 4);
            }
        }

        // 每隔10秒打印一次吞吐量
        time_t now = time(NULL);
        if (now - start_time >= 10) {
            double mbps = (double)total_bytes * 8 / (1024 * 1024 * (now - start_time));
            printf("Throughput: %.2f Mbps\n", mbps);
            total_bytes = 0;
            start_time = now;
        }
    }

    close(sockfd);
    return 0;
}

