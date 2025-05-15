// key_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <time.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/bpf.h>

// 密钥数据结构
struct sm4_key_payload {
    uint32_t key[4];
    uint32_t key_id;
    uint64_t timestamp;
    uint64_t expiration;
};

// BPF Map中使用的密钥结构
struct sm4_key {
    uint32_t key[4];
    uint32_t key_id;
    uint64_t timestamp;
};

// 全局变量
int running = 1;
const int XDP_MAP_ID = 7;  // XDP程序使用的Map ID

// 信号处理函数
void handle_signal(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    running = 0;
}

// 打印密钥信息
void print_key_info(struct sm4_key_payload *key_data) {
    printf("Key ID: %u\n", key_data->key_id);
    printf("Key: 0x%08x 0x%08x 0x%08x 0x%08x\n", 
           key_data->key[0], key_data->key[1], 
           key_data->key[2], key_data->key[3]);
    printf("Generated: %s", ctime((time_t*)&key_data->timestamp));
    printf("Expires: %s", ctime((time_t*)&key_data->expiration));
    printf("Valid for: %lu seconds\n", 
           key_data->expiration - time(NULL));
}

// 请求密钥的函数
int request_key(const char *server_ip, int port, struct sm4_key_payload *key_data) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        return -1;
    }
    
    // 设置连接超时
    struct timeval tv;
    tv.tv_sec = 5;  // 5秒超时
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt failed");
    }
    
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return -1;
    }
    
    // 接收密钥数据
    ssize_t bytes_received = recv(sock, key_data, sizeof(*key_data), 0);
    if (bytes_received != sizeof(*key_data)) {
        perror("Receive failed");
        close(sock);
        return -1;
    }
    
    close(sock);
    return 0;
}

// 创建BPF目录
int ensure_bpf_dir_exists() {
    struct stat st = {0};
    if (stat("/sys/fs/bpf", &st) == -1) {
        fprintf(stderr, "BPF filesystem not mounted. Try: mount -t bpf bpf /sys/fs/bpf/\n");
        return -1;
    }
    
    // 确保目录存在
    const char *map_dir = "/sys/fs/bpf/sm4_key_encrypt";
    if (stat(map_dir, &st) == -1) {
        printf("Creating directory: %s\n", map_dir);
        if (mkdir(map_dir, 0700) < 0) {
            perror("Failed to create directory");
            return -1;
        }
    }
    
    return 0;
}

// 创建新的BPF Map
int create_new_bpf_map(const char *map_path) {
    // 确保BPF目录存在
    if (ensure_bpf_dir_exists() < 0) {
        return -1;
    }
    
    printf("Creating new BPF map at: %s\n", map_path);
    
    // 确保目录存在
    char dir_path[256];
    strncpy(dir_path, map_path, sizeof(dir_path));
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        printf("Ensuring directory exists: %s\n", dir_path);
        mkdir(dir_path, 0755);  // 使用更宽松的权限
    }
    
    // 创建BPF Map
    int map_fd = bpf_create_map(BPF_MAP_TYPE_ARRAY, sizeof(uint32_t), sizeof(struct sm4_key), 1, 0);
    if (map_fd < 0) {
        printf("Failed to create BPF map, error: %s\n", strerror(errno));
        return -1;
    }
    
    printf("Map created successfully, fd = %d\n", map_fd);
    
    // Pin BPF Map到文件系统
    printf("Pinning map to: %s\n", map_path);
    if (bpf_obj_pin(map_fd, map_path) < 0) {
        printf("Failed to pin BPF map, error: %s\n", strerror(errno));
        close(map_fd);
        return -1;
    }
    
    printf("Created and pinned BPF map at %s\n", map_path);
    return map_fd;
}

// 确保BPF Map存在
int ensure_bpf_map_exists(const char *map_path) {
    // 尝试打开已存在的map
    printf("Trying to get BPF map at: %s\n", map_path);
    int map_fd = bpf_obj_get(map_path);
    if (map_fd >= 0) {
        printf("Successfully opened existing BPF map, fd = %d\n", map_fd);
        return map_fd; // Map已存在
    }
    
    printf("Error opening map: %s\n", strerror(errno));
    
    // 如果map不存在，创建新的
    printf("BPF map not found at %s, creating new one\n", map_path);
    return create_new_bpf_map(map_path);
}

// 更新Pin路径的BPF Map
int update_bpf_map_key(const char *map_path, struct sm4_key_payload *key_data) {
    // 确保map存在
    printf("Trying to access BPF map at: %s\n", map_path);
    int map_fd = ensure_bpf_map_exists(map_path);
    if (map_fd < 0) {
        printf("Failed to get map fd, error: %s\n", strerror(errno));
        return -1;
    }
    printf("Successfully opened BPF map, fd = %d\n", map_fd);
    
    // 转换为BPF map使用的密钥格式
    struct sm4_key bpf_key;
    memcpy(bpf_key.key, key_data->key, sizeof(bpf_key.key));
    bpf_key.key_id = key_data->key_id;
    bpf_key.timestamp = time(NULL);
    
    printf("Updating pinned BPF map with key_id: %u\n", bpf_key.key_id);
    printf("Key: 0x%08x 0x%08x 0x%08x 0x%08x\n", 
           bpf_key.key[0], bpf_key.key[1], bpf_key.key[2], bpf_key.key[3]);
    
    uint32_t key_idx = 0;  // 使用固定索引0
    if (bpf_map_update_elem(map_fd, &key_idx, &bpf_key, BPF_ANY)) {
        printf("Failed to update BPF map, error: %s\n", strerror(errno));
        close(map_fd);
        return -1;
    }
    
    printf("Map update operation completed\n");
    
    // 读回密钥以确认更新成功
    struct sm4_key verify_key;
    if (bpf_map_lookup_elem(map_fd, &key_idx, &verify_key) == 0) {
        printf("Read back key_id: %u\n", verify_key.key_id);
        printf("Read back key: 0x%08x 0x%08x 0x%08x 0x%08x\n", 
               verify_key.key[0], verify_key.key[1], verify_key.key[2], verify_key.key[3]);
        
        if (verify_key.key_id == bpf_key.key_id) {
            printf("Verified key ID %u is now in pinned BPF map\n", verify_key.key_id);
        } else {
            printf("Warning: Key verification failed! Map has key ID %u\n", verify_key.key_id);
        }
    } else {
        printf("Failed to read back key, error: %s\n", strerror(errno));
    }
    
    close(map_fd);
    return 0;
}

// 直接通过ID更新BPF Map
int update_bpf_map_key_by_id(int map_id, struct sm4_key_payload *key_data) {
    printf("Trying to access BPF map with ID: %d\n", map_id);
    int map_fd = bpf_map_get_fd_by_id(map_id);
    if (map_fd < 0) {
        printf("Failed to get map fd for ID %d, error: %s\n", map_id, strerror(errno));
        return -1;
    }
    
    // 获取map信息
    struct bpf_map_info info = {};
    uint32_t info_len = sizeof(info);
    if (bpf_obj_get_info_by_fd(map_fd, &info, &info_len) == 0) {
        printf("Map ID %d info: name=%s, type=%d, key_size=%d, value_size=%d\n", 
               map_id, info.name, info.type, info.key_size, info.value_size);
    }
    
    // 转换为BPF map使用的密钥格式
    struct sm4_key bpf_key;
    memcpy(bpf_key.key, key_data->key, sizeof(bpf_key.key));
    bpf_key.key_id = key_data->key_id;
    bpf_key.timestamp = time(NULL);
    
    printf("Updating BPF map ID %d with key_id: %u\n", map_id, bpf_key.key_id);
    printf("Key: 0x%08x 0x%08x 0x%08x 0x%08x\n", 
           bpf_key.key[0], bpf_key.key[1], bpf_key.key[2], bpf_key.key[3]);
    
    uint32_t key_idx = 0;  // 使用固定索引0
    if (bpf_map_update_elem(map_fd, &key_idx, &bpf_key, BPF_ANY)) {
        printf("Failed to update BPF map with ID %d, error: %s\n", map_id, strerror(errno));
        close(map_fd);
        return -1;
    }
    
    printf("Map update operation completed\n");
    
    // 读回密钥以确认更新成功
    struct sm4_key verify_key;
    if (bpf_map_lookup_elem(map_fd, &key_idx, &verify_key) == 0) {
        printf("Read back key_id: %u from map ID %d\n", verify_key.key_id, map_id);
        printf("Read back key: 0x%08x 0x%08x 0x%08x 0x%08x\n", 
               verify_key.key[0], verify_key.key[1], verify_key.key[2], verify_key.key[3]);
        
        if (verify_key.key_id == bpf_key.key_id) {
            printf("Verified key ID %u is now in BPF map with ID %d\n", verify_key.key_id, map_id);
        } else {
            printf("Warning: Key verification failed! Map has key ID %u\n", verify_key.key_id);
        }
    } else {
        printf("Failed to read back key, error: %s\n", strerror(errno));
    }
    
    close(map_fd);
    return 0;
}

// 列出所有BPF Maps并尝试更新密钥
void list_all_maps_and_try_update(struct sm4_key_payload *key_data) {
    printf("\nListing all BPF maps in the system:\n");
    system("bpftool map list");
    
    printf("\nLooking for maps with names containing 'sm4' or 'key':\n");
    system("bpftool map list | grep -i 'sm4\\|key'");
    
    printf("\nChecking XDP programs:\n");
    system("bpftool prog list | grep xdp");
    
    printf("\nChecking network interfaces with XDP programs:\n");
    system("bpftool net");
}

// 显示当前BPF Map状态
void show_maps_status() {
    printf("\nCurrent BPF maps status:\n");
    
    printf("\n1. By name 'sm4_key_map':\n");
    system("bpftool map dump name sm4_key_map");
    
    printf("\n2. ID %d (XDP program's map):\n", XDP_MAP_ID);
    char cmd[100];
    snprintf(cmd, sizeof(cmd), "bpftool map dump id %d", XDP_MAP_ID);
    system(cmd);
    
    printf("\n3. Pinned map at /sys/fs/bpf/sm4_key_encrypt/sm4_key_map:\n");
    system("bpftool map dump pinned /sys/fs/bpf/sm4_key_encrypt/sm4_key_map");
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <kms_server_ip> <bpf_map_path> [poll_interval] [port]\n", argv[0]);
        printf("Example: %s 192.168.58.132 /sys/fs/bpf/sm4_key_encrypt/sm4_key_map 60 9000\n", argv[0]);
        return 1;
    }
    
    const char *server_ip = argv[1];
    const char *map_path = argv[2];
    
    // 可选参数：轮询间隔(默认60秒)
    int poll_interval = 60;
    if (argc > 3) {
        poll_interval = atoi(argv[3]);
        if (poll_interval < 10) poll_interval = 10; // 最小10秒
    }
    
    // 可选参数：KMS服务器端口(默认9000)
    int port = 9000;
    if (argc > 4) {
        port = atoi(argv[4]);
    }
    
    // 设置信号处理
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    printf("Starting SM4 key client\n");
    printf("Server: %s:%d\n", server_ip, port);
    printf("BPF Map Path: %s\n", map_path);
    printf("XDP Map ID: %d\n", XDP_MAP_ID);
    printf("Poll interval: %d seconds\n", poll_interval);
    
    // 记录上一次接收的密钥ID
    uint32_t last_key_id = 0;
    
    // 系统信息
    printf("\nSystem BPF information:\n");
    system("mount | grep bpf");
    system("ls -la /sys/fs/bpf/");
    
    // 初始列出所有Maps
    list_all_maps_and_try_update(NULL);
    
    // 显示初始Map状态
    show_maps_status();
    
    while (running) {
        struct sm4_key_payload key_data;
        
        // 请求新密钥
        printf("\nRequesting key from KMS server...\n");
        if (request_key(server_ip, port, &key_data) == 0) {
            printf("Received key:\n");
            print_key_info(&key_data);
            
            // 首先尝试通过XDP使用的Map ID更新密钥
            printf("\nAttempting to update BPF map with ID %d (XDP program's map)...\n", XDP_MAP_ID);
            if (update_bpf_map_key_by_id(XDP_MAP_ID, &key_data) == 0) {
                printf("Successfully updated key in BPF map with ID %d\n", XDP_MAP_ID);
                last_key_id = key_data.key_id;
            } else {
                printf("Failed to update BPF map with ID %d\n", XDP_MAP_ID);
            }
            
            // 然后也更新pinned路径的Map
            printf("\nAttempting to update pinned BPF map at %s...\n", map_path);
            if (update_bpf_map_key(map_path, &key_data) == 0) {
                printf("Successfully updated key in pinned BPF map\n");
            } else {
                printf("Failed to update pinned BPF map\n");
            } 
            
            // 检查当前更新状态
            show_maps_status();
        } else {
            printf("Failed to connect to KMS server\n");
        }
        
        // 等待下一次轮询
        printf("\nSleeping for %d seconds...\n", poll_interval);
        for (int i = 0; i < poll_interval && running; i++) {
            sleep(1);
        }
        
        if (!running) break;
    }
    
    printf("Key client shutting down\n");
    return 0;
}
