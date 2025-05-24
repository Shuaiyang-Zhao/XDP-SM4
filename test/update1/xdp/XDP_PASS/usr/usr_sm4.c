#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "sm4.h"

#define UDP_PORT 1234
#define MAX_BUFFER 2048
#define BLOCK_SIZE 16

// 简单打印16进制数据函数
void print_hex(const unsigned char *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 8 == 0)
            printf(" ");
    }
    printf("\n");
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    unsigned char buffer[MAX_BUFFER];

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

    while (1) {
        ssize_t recv_len = recvfrom(sockfd, buffer, MAX_BUFFER, 0,
                                    (struct sockaddr *)&client_addr, &client_addr_len);
        if (recv_len < 0) {
            perror("recvfrom error");
            continue;
        }
        printf("\nReceived %ld bytes from %s:%d\n", recv_len,
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        print_hex(buffer, recv_len);

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
            // 将 16 字节数据转换为 4 个 u32 值，进行字节序转换
            for (int j = 0; j < 4; j++) {
                u32 tmp;
                memcpy(&tmp, data + i + j * 4, 4);
                X[j] = ntohl(tmp);    // 网络字节序转换为主机字节序
            }
            encryptSM4(X, RK, Y);
            // 将加密结果按网络字节序保存回输出缓存
            for (int j = 0; j < 4; j++) {
                u32 tmp = htonl(Y[j]);
                memcpy(encrypted + i + j * 4, &tmp, 4);
            }
        }
        
        printf("Encrypted data:\n");
        print_hex(encrypted, total_len);

        // 将加密后的数据返回给发送端
        ssize_t sent = sendto(sockfd, encrypted, total_len, 0,
                              (struct sockaddr *)&client_addr, client_addr_len);
        if (sent < 0) {
            perror("sendto error");
        }

        free(data);
        free(encrypted);
    }

    close(sockfd);
    return 0;
}
