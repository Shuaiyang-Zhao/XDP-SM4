// kms_server.c
// kms_server -r
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>

#define KEY_LIFETIME 300

struct sm4_key_payload {
    uint32_t key[4];       
    uint32_t key_id;        
    uint64_t timestamp;    
    uint64_t expiration;    // 过期时间戳
};

int server_socket = -1;
int running = 1;

void handle_signal(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    running = 0;
    if (server_socket != -1) {
        close(server_socket);
    }
    exit(0);
}

void generate_test_key(uint32_t key[4]) {
    // 使用固定密钥进行测试，简化调试
  
    key[3] = 0xfedcba98;
    key[0] = 0x76543210;
    key[2] = 0x89abcdef;
    key[1] = 0x01234567;
   
    
    printf("Using test key\n");
}

// 生成随机密钥(实际生产环境使用)
void generate_random_key(uint32_t key[4]) {
    // Linux 系统随机设备 /dev/urandom，用于获取高质量的伪随机数
    int urandom_fd = open("/dev/urandom", O_RDONLY);
    if (urandom_fd < 0) {
        perror("Failed to open /dev/urandom");
        exit(1);
    }
    
    ssize_t bytes_read = read(urandom_fd, key, 16);
    if (bytes_read != 16) {
        perror("Failed to read random data");
        exit(1);
    }
    
    close(urandom_fd);
}

// 生成新的密钥并设置其属性
void create_new_key(struct sm4_key_payload *key_data, int use_fixed_key) {
    static uint32_t next_key_id = 1;
    
    // 根据配置选择使用固定密钥还是随机密钥
    if (use_fixed_key) {
        generate_test_key(key_data->key);
    } else {
        generate_random_key(key_data->key);
    }
    
    // 设置密钥ID和时间戳
    key_data->key_id = next_key_id++;
    key_data->timestamp = time(NULL);
    key_data->expiration = key_data->timestamp + KEY_LIFETIME;
    
    printf("Generated new key ID: %u\n", key_data->key_id);
    printf("Key: %08x %08x %08x %08x\n", 
           key_data->key[0], key_data->key[1], 
           key_data->key[2], key_data->key[3]);
    printf("Timestamp: %s", ctime((time_t*)&key_data->timestamp));
    printf("Expiration: %s", ctime((time_t*)&key_data->expiration));
}

/**
struct sockaddr_in {
    sa_family_t    sin_family; // 地址族，AF_INET表示IPv4
    in_port_t      sin_port;   // 端口号（网络字节序）
    struct in_addr sin_addr;   // IP地址（网络字节序）
    char           sin_zero[8];// 填充用，保证结构体大小与sockaddr相同
};
 */

// 主服务器程序
int main(int argc, char **argv) {
    int client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int use_fixed_key = 1;  // 默认使用固定密钥进行测试
    int port = 9000;        // 默认端口
    
    // 设置信号处理
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    // 解析命令行参数
    int opt;
    while ((opt = getopt(argc, argv, "rp:")) != -1) {
        switch (opt) {
            case 'r':
                use_fixed_key = 0;
                printf("Using random keys for production\n");
                break;
            case 'p':
                port = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-r] [-p port]\n", argv[0]);
                fprintf(stderr, "  -r         Use random keys instead of fixed test key\n");
                fprintf(stderr, "  -p port    Specify port number (default: 9000)\n");
                exit(1);
        }
    }
    
    if (use_fixed_key) {
        printf("Using fixed test key (use -r for random keys)\n");
    }
    
    // 创建并保存密钥
    struct sm4_key_payload current_key;
    create_new_key(&current_key, use_fixed_key);
    
    // 创建套接字
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating socket");
        exit(1);
    }
    
    // 设置套接字选项
    int opt_val = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val)) < 0) {
        perror("setsockopt failed");
        exit(1);
    }
    
    // 绑定地址和端口
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    
    // 监听连接
    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        exit(1);
    }
    
    printf("KMS server started on port %d\n", port);
    printf("Waiting for clients...\n");
    
    // 周期性更新密钥
    time_t last_key_update = time(NULL);
    
    while (running) {
        fd_set read_fds;
        struct timeval timeout;
        
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);
        
        // 设置1秒超时，用于周期性检查密钥是否需要更新
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        /*
        监听多个文件描述符（本例中只有 server_socket）是否准备好进行 I/O 操作：
        第一个参数：监听的最大文件描述符值 + 1；
        第二个参数：关心“可读”的 socket；
        第三/四个参数设为 NULL，不关心“可写”和“异常”；
        第五个参数：超时时间（1 秒）；
        */

        int activity = select(server_socket + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            if (errno == EINTR) {
                // 被信号中断，检查是否需要退出
                if (!running) break;
                continue;
            }
            perror("Select error");
            continue;
        }
        
        // 检查是否需要更新密钥(只有在不使用固定密钥时才更新)
        if (!use_fixed_key) {
            time_t now = time(NULL);
            if (now - last_key_update >= KEY_LIFETIME/2) {
                create_new_key(&current_key, use_fixed_key);
                last_key_update = now;
            }
        }
        
        // 处理新连接
        if (activity > 0 && FD_ISSET(server_socket, &read_fds)) {
            client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
            if (client_socket < 0) {
                perror("Accept failed");
                continue;
            }
            
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);//二进制转字符串
            printf("New connection from %s:%d\n", client_ip, ntohs(client_addr.sin_port));
            
            // 发送当前密钥给客户端
            if (send(client_socket, &current_key, sizeof(current_key), 0) != sizeof(current_key)) {
                perror("Send failed");
            } else {
                printf("Sent key ID %u to %s\n", current_key.key_id, client_ip);
            }
            
            close(client_socket);
        }
    }
    
    printf("KMS server shutting down\n");
    if (server_socket != -1) {
        close(server_socket);
    }
    
    return 0;
}


