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
#include <time.h>

#define PORT_ENCRYPT 1234
#define PORT_DECRYPT 1233
#define LOCAL_PORT_ENCRYPT 1235
#define LOCAL_PORT_DECRYPT 1236

#define ENCRYPT_IP "192.168.58.129"
#define DECRYPT_IP "192.168.58.128"

unsigned char MESSAGE[] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32
};

void print_hex(const char *title, const unsigned char *data, size_t length) {
    printf("%s [%zu 字节]: ", title, length);
    for (size_t i = 0; i < length; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 8 == 0) printf(" ");
    }
    printf("\n");
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
    int sock_encrypt, sock_decrypt;
    struct sockaddr_in b_addr, c_addr;
    unsigned char buf[2048];
    ssize_t n;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    size_t plain_len = sizeof(MESSAGE);  // ✅ 记录原始明文长度

    printf("UDP 客户端启动，使用两个 socket。\n");

    sock_encrypt = bind_socket(LOCAL_PORT_ENCRYPT);
    sock_decrypt = bind_socket(LOCAL_PORT_DECRYPT);

    memset(&b_addr, 0, sizeof(b_addr));
    b_addr.sin_family = AF_INET;
    b_addr.sin_port = htons(PORT_ENCRYPT);
    inet_pton(AF_INET, ENCRYPT_IP, &b_addr.sin_addr);

    memset(&c_addr, 0, sizeof(c_addr));
    c_addr.sin_family = AF_INET;
    c_addr.sin_port = htons(PORT_DECRYPT);
    inet_pton(AF_INET, DECRYPT_IP, &c_addr.sin_addr);

    print_hex("📤 发送原始明文给 B", MESSAGE, plain_len);
    sendto(sock_encrypt, MESSAGE, plain_len, 0,
           (struct sockaddr*)&b_addr, sizeof(b_addr));

    time_t start = time(NULL);
    while (1) {
        n = recvfrom(sock_encrypt, buf, sizeof(buf), 0, NULL, NULL);
        if (n > 0) {
            print_hex("📥 从 B 收到密文", buf, n);
            printf("➡️  正在将密文转发给 C ...\n");

            sendto(sock_decrypt, buf, n, 0, (struct sockaddr*)&c_addr, sizeof(c_addr));
            break;
        }

        if (time(NULL) - start > 5) {
            fprintf(stderr, "❌ 超时：未收到来自 B 的密文响应。\n");
            goto cleanup;
        }

        usleep(100 * 1000);
    }

    start = time(NULL);
    while (1) {
        n = recvfrom(sock_decrypt, buf, sizeof(buf), 0, NULL, NULL);
        if (n > 0) {
            // ✅ 只输出原始长度部分
            print_hex("📥 从 C 收到解密结果", buf, plain_len);
            break;
        }

        if (time(NULL) - start > 5) {
            fprintf(stderr, "❌ 超时：未收到来自 C 的解密响应。\n");
            break;
        }

        usleep(100 * 1000);
    }

cleanup:
    close(sock_encrypt);
    close(sock_decrypt);
    printf("✅ 本轮加解密通信完成。\n");
    return 0;
}

