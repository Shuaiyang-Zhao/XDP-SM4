//  UDP4 发包程序, 测试最大负载 1472 字节

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 1233        // 目标端口
#define SOURCE_PORT 1235  // 源端口
#define DEST_IP "192.168.58.128"

unsigned char MESSAGE[] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
    };


    void print_hex(const unsigned char *data, size_t length) {
    printf("Hex data [%zu bytes]: ", length);
    for (size_t i = 0; i < length; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 8 == 0) printf(" "); 
    }
    printf("\n");
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr, localaddr;
    int optval = 1;  // 用于设置 SO_REUSEADDR

    printf("Initializing IPv4 UDP sender...\n");

    // 创建UDP套接字
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("[ERROR] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 设置端口重用
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("[ERROR] setsockopt failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 绑定源端口
    memset(&localaddr, 0, sizeof(localaddr));
    localaddr.sin_family = AF_INET;
    localaddr.sin_port = htons(SOURCE_PORT);  // 设置源端口
    localaddr.sin_addr.s_addr = INADDR_ANY;   // 使用本机的任意地址

    if (bind(sockfd, (struct sockaddr*)&localaddr, sizeof(localaddr)) < 0) {
        perror("[ERROR] Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 设置目标服务器地址和端口
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);  // 目标端口
    inet_pton(AF_INET, DEST_IP, &servaddr.sin_addr);  

    printf("Configured destination: %s:%d\n", DEST_IP, PORT);
    print_hex(MESSAGE, sizeof(MESSAGE));

    int counter = 1;
    while (1) {
        // 发送数据
        ssize_t sent = sendto(sockfd, MESSAGE, sizeof(MESSAGE), 0,
                              (struct sockaddr*)&servaddr, sizeof(servaddr));

        if (sent < 0) {
            perror("[ERROR] Send failed");
        } else {
            printf("[%04d] Sent %zd bytes to %s:%d\n", 
                  counter++, sent, DEST_IP, PORT);
            print_hex(MESSAGE, sent);
        }

        sleep(1);  // 每秒发送一次
    }

    close(sockfd);
    return 0;
}

