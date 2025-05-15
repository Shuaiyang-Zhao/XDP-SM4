#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 1233
#define MAXLINE 1600
#define LISTEN_IP "192.168.58.133"

void print_hex(const unsigned char *data, size_t length) {
    printf("Hex data [%zu bytes]: ", length);
    for(size_t i = 0; i < length; i++) {
        printf("%02x ", data[i]);
        if((i+1) % 8 == 0) printf(" "); // 每8字节加空格分隔
    }
    printf("\n");
}

int main() {
    int sockfd;
    char buffer[MAXLINE];
    struct sockaddr_in servaddr, cliaddr;

    printf("Initializing IPv4 UDP server...\n");
    
    // 创建UDP套接字
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("[ERROR] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 设置端口复用选项
    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("[ERROR] SO_REUSEADDR failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        perror("[ERROR] SO_REUSEPORT failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    inet_pton(AF_INET, LISTEN_IP, &servaddr.sin_addr);

    printf("Binding to %s:%d...\n", LISTEN_IP, PORT);
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("[ERROR] Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server is ready and listening...\n");
    
    int counter = 1;
    socklen_t len = sizeof(cliaddr);
    
    while(1) {
        // 接收数据
        ssize_t n = recvfrom(sockfd, buffer, MAXLINE, MSG_WAITALL,
                            (struct sockaddr *)&cliaddr, &len);
        
        if(n < 0) {
            perror("[WARN] Receive failed");
            continue;
        }

        // 解析客户端信息
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cliaddr.sin_addr, client_ip, sizeof(client_ip));
        unsigned short client_port = ntohs(cliaddr.sin_port);

        // 打印接收信息
        printf("[%04d] Received %zd bytes from %s:%d\n",
              counter++, n, client_ip, client_port);
        print_hex((unsigned char*)buffer, n);

        /* 响应功能可在此处添加
        if (sendto(sockfd, "ACK", 3, 0, 
                 (struct sockaddr *)&cliaddr, len) < 0) {
            perror("[WARN] Send response failed");
        }
        */
    }

    close(sockfd);
    return 0;
}

