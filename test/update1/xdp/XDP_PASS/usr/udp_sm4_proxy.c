// udp_sm4_proxy.c: SM4-encrypted UDP proxy between iperf3 client (B) and iperf3 server (A)
// Usage: compile with: gcc -o udp_sm4_proxy udp_sm4_proxy.c sm4.o -lrt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>
#include "sm4.h"

#define LISTEN_PORT 1240 // proxy listens here (from B)
#define SERVER_PORT 1239 // iperf3 server port (on localhost)
#define MAX_BUFFER 2048
#define BLOCK_SIZE 16
#define TIMEOUT_SEC 1 // timeout for server response

// print hex for debug
static void print_hex(const unsigned char *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        printf("%02x", data[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
        else
            printf(" ");
    }
    if (len % 16)
        printf("\n");   
}

int main()
{
    int sockfd;
    struct sockaddr_in listen_addr, client_addr, server_addr;
    socklen_t client_len = sizeof(client_addr);
    unsigned char buffer[MAX_BUFFER];
    unsigned char work_buf[MAX_BUFFER + BLOCK_SIZE];
    unsigned char crypt_buf[MAX_BUFFER + BLOCK_SIZE];

    // create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // enable reuse
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // bind to LISTEN_PORT
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    listen_addr.sin_port = htons(LISTEN_PORT);
    if (bind(sockfd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0)
    {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Proxy listening on UDP port %d\n", LISTEN_PORT);

    // prepare iperf3 server address (localhost)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_addr.sin_port = htons(SERVER_PORT);

    // SM4 key schedule
    u32 MK[4] = {0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210};
    u32 K[4], RK[32];
    getRK(MK, K, RK);

    // set recv timeout for server responses
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (1)
    {
        // 1) receive from client (B)
        ssize_t recv_len = recvfrom(sockfd, buffer, MAX_BUFFER, 0,
                                    (struct sockaddr *)&client_addr, &client_len);
        if (recv_len < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue; // timeout, loop
            perror("recvfrom client");
            continue;
        }
        // printf("Received %zd bytes from %s:%d\n", recv_len,
        //    inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        // print_hex(buffer, recv_len);

        // 2) pad to BLOCK_SIZE
        size_t pad = (BLOCK_SIZE - (recv_len % BLOCK_SIZE)) % BLOCK_SIZE;
        size_t total = recv_len + pad;
        memcpy(work_buf, buffer, recv_len);
        if (pad)
            memset(work_buf + recv_len, 0, pad);

        // 3) SM4 encrypt blockwise
        for (size_t off = 0; off < total; off += BLOCK_SIZE)
        {
            u32 X[4], Y[4];
            for (int j = 0; j < 4; j++)
            {
                u32 tmp;
                memcpy(&tmp, work_buf + off + j * 4, 4);
                X[j] = ntohl(tmp);
            }
            encryptSM4(X, RK, Y);
            // for (int j = 0; j < 4; j++) {
            //     u32 tmp = htonl(Y[j]);
            //     memcpy(crypt_buf + off + j*4, &tmp, 4);
            // }
        }
        // print_hex(crypt_buf, total);

        // 4) forward encrypted to iperf3 server
        // if (sendto(sockfd, crypt_buf, total, 0,
        //            (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        // {
        //     perror("sendto server");
        //     continue;
        // }

        if (sendto(sockfd, buffer, recv_len, 0,
                   (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            perror("sendto server");
            continue;
        }
        // 5) wait for server response (optional)
        // ssize_t resp_len = recvfrom(sockfd, buffer, MAX_BUFFER, 0, NULL, NULL);
        // if (resp_len > 0)
        // {
        //     // printf("Received %zd bytes from server\n", resp_len);
        //     // decrypt response if needed (mirror of encrypt)
        //     // Here assuming symmetric encryption both ways
        //     size_t resp_pad = (BLOCK_SIZE - (resp_len % BLOCK_SIZE)) % BLOCK_SIZE;
        //     size_t resp_total = resp_len + resp_pad;
        //     memcpy(work_buf, buffer, resp_len);
        //     if (resp_pad)
        //         memset(work_buf + resp_len, 0, resp_pad);
        //     for (size_t off = 0; off < resp_total; off += BLOCK_SIZE)
        //     {
        //         u32 X[4], Y[4];
        //         for (int j = 0; j < 4; j++)
        //         {
        //             u32 tmp;
        //             memcpy(&tmp, work_buf + off + j * 4, 4);
        //             X[j] = ntohl(tmp);
        //         }
        //         decryptSM4(X, RK, Y);
        //         for (int j = 0; j < 4; j++)
        //         {
        //             u32 tmp = htonl(Y[j]);
        //             memcpy(crypt_buf + off + j * 4, &tmp, 4);
        //         }
        //     }
        //     // send decrypted back to client
        //     if (sendto(sockfd, crypt_buf, resp_len, 0,
        //                (struct sockaddr *)&client_addr, client_len) < 0)
        //     {
        //         perror("sendto client");
        //     }
        // }
        ssize_t resp_len = recvfrom(sockfd, buffer, MAX_BUFFER, 0, NULL, NULL);
        if (resp_len > 0)
        {
            // 直接转发原始响应
            if (sendto(sockfd, buffer, resp_len, 0,
                       (struct sockaddr *)&client_addr, client_len) < 0)
            {
                perror("sendto client");
            }
        }
    }
    close(sockfd);
    return 0;
}
