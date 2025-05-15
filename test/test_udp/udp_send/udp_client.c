// ./udp_client ./config/config1.json ./data/data15.txt

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <sys/resource.h>
#include <cjson/cJSON.h>
#include <stdatomic.h>

atomic_int sent_packet_count = 0;

#define MAX_DATA_SIZE 1600

typedef struct
{
    char dest_ip[64];
    int dest_port;
    int local_port;
    int thread_num;
    int max_packet_num;
    int data_size;
    unsigned char send_data[MAX_DATA_SIZE];
    int debug;
} config_t;

typedef struct
{
    uint32_t seq;
    uint64_t timestamp;
    unsigned char send_data[]; // flexible array
} __attribute__((packed)) packet_t;

typedef struct {
    uint32_t base_seq;
    uint32_t packets_to_send;
} thread_arg_t;

int sockfd;
struct sockaddr_in dest_addr, local_addr;
config_t config;

uint64_t get_timestamp_ns()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)(ts.tv_sec * 1000000000ULL + ts.tv_nsec);
}

uint64_t get_wall_time_us()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
}

long get_cpu_time_microseconds()
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_utime.tv_sec * 1000000L + usage.ru_utime.tv_usec +
           usage.ru_stime.tv_sec * 1000000L + usage.ru_stime.tv_usec;
}

void print_packet(packet_t *pkt, const char *prefix)
{
    printf("[%s] 序号: %u, 时间戳: %" PRIu64 ", 数据: ", prefix, pkt->seq, pkt->timestamp);
    for (int i = 0; i < config.data_size; ++i)
        printf("%02x ", pkt->send_data[i]);
    printf("\n");
}

void *sender_thread(void *arg)
{
    thread_arg_t *targ = (thread_arg_t *)arg;
    uint32_t base_seq = targ->base_seq;
    uint32_t packets = targ->packets_to_send;

   int pkt_size = sizeof(packet_t) + config.data_size;
    packet_t *pkt = (packet_t *)malloc(pkt_size);
    memcpy(pkt->send_data, config.send_data, config.data_size);

    for (uint32_t i = 0; i < packets; ++i)
    {
        pkt->seq = base_seq + i;
        pkt->timestamp = get_timestamp_ns();

        sendto(sockfd, pkt, pkt_size, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (config.debug)
            print_packet(pkt, "客户端发送");

        atomic_fetch_add(&sent_packet_count, 1);
    }

    free(pkt);
    free(targ);  // 动态申请的结构体也要释放
    return NULL;
}

int hex_to_bytes(const char *hex, unsigned char *out, int max_len)
{
    int len = strlen(hex) / 2;
    if (len > max_len)
        len = max_len;

    for (int i = 0; i < len; ++i)
        sscanf(hex + 2 * i, "%2hhx", &out[i]);

    return len;
}

void load_config(const char *json_path, const char *data_path, config_t *cfg)
{
    // 加载主配置文件
    FILE *fp = fopen(json_path, "rb");
    if (!fp)
    {
        perror("无法打开主配置文件");
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);
    
    char *data = (char *)malloc(size + 1);
    if (fread(data, 1, size, fp) != size)
    {
        fprintf(stderr, "读取配置文件失败\n");
        exit(1);
    }

    data[size] = '\0';
    fclose(fp);

    cJSON *json = cJSON_Parse(data);
    if (!json)
    {
        fprintf(stderr, "解析 JSON 配置失败\n");
        exit(1);
    }

    strcpy(cfg->dest_ip, cJSON_GetObjectItem(json, "dest_ip")->valuestring);
    cfg->dest_port = cJSON_GetObjectItem(json, "dest_port")->valueint;
    cfg->local_port = cJSON_GetObjectItem(json, "local_port")->valueint;
    cfg->thread_num = cJSON_GetObjectItem(json, "thread_num")->valueint;
    cfg->max_packet_num = cJSON_GetObjectItem(json, "max_packet_num")->valueint;
    cfg->debug = cJSON_GetObjectItem(json, "debug")->valueint;

    cJSON_Delete(json);
    free(data);

    // 加载数据文件
    FILE *dfp = fopen(data_path, "r");
    if (!dfp)
    {
        perror("无法打开数据配置文件");
        exit(1);
    }

    char line[5000];
    while (fgets(line, sizeof(line), dfp))
    {
        if (strncmp(line, "data_size=", 10) == 0)
        {
            cfg->data_size = atoi(line + 10);
        }
        else if (strncmp(line, "send_data=", 10) == 0)
        {
            char hex_str[5000]; // 创建可修改的缓冲区
            strncpy(hex_str, line + 10, sizeof(hex_str) - 1);
            hex_str[sizeof(hex_str) - 1] = '\0';      // 确保 null 结尾
            hex_str[strcspn(hex_str, "\r\n")] = '\0'; // 去除换行符

            printf("Hex data: %s\n", hex_str);  // 在读取 `send_data` 后添加此行调试代码

            int bytes = hex_to_bytes(hex_str, cfg->send_data, MAX_DATA_SIZE);
        printf("Hex data: %s\n", hex_str);  // 在读取 `send_data` 后添加此行调试代码

            printf("\n");

            if (bytes != cfg->data_size)
            {
                fprintf(stderr, "数据长度 (%d) 与 data_size (%d) 不一致\n", bytes, cfg->data_size);
                exit(1);
            }
        }
    }
    fclose(dfp);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "用法: %s <主配置文件路径> <数据配置文件路径>\n", argv[0]);
        exit(1);
    }

    const char *config_path = argv[1];
    const char *data_path = argv[2];

    load_config(config_path, data_path, &config);

    if (argc > 3)
    {
        if (strncmp(argv[3], "debug=true", 10) == 0)
        {
            config.debug = 1;
        }
        else if (strncmp(argv[3], "debug=false", 11) == 0)
        {
            config.debug = 0;
        }
    }

    long wall_start = get_wall_time_us();
    long cpu_start = get_cpu_time_microseconds();

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("创建套接字失败");
        exit(EXIT_FAILURE);
    }
    // 设置 SO_REUSEADDR 和 SO_REUSEPORT
int reuse = 1;
if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
{
    perror("设置 SO_REUSEADDR 失败");
    exit(EXIT_FAILURE);
}


    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(config.local_port);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
    {
        perror("绑定失败");
        exit(EXIT_FAILURE);
    }

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(config.dest_port);
    inet_pton(AF_INET, config.dest_ip, &dest_addr.sin_addr);

    pthread_t threads[config.thread_num];
    int base_count = config.max_packet_num / config.thread_num;
    int extra = config.max_packet_num % config.thread_num;
    
    uint32_t seq = 0;
    for (int i = 0; i < config.thread_num; ++i)
    {
        thread_arg_t *arg = malloc(sizeof(thread_arg_t));
        arg->packets_to_send = base_count + (i < extra ? 1 : 0);
        arg->base_seq = seq;
        seq += arg->packets_to_send;
    
        int ret = pthread_create(&threads[i], NULL, sender_thread, arg);
        if (ret != 0)
        {
            perror("线程创建失败");
            exit(1);
        }
    }

    // 用于判断是否所有包已经发送完
    while (atomic_load(&sent_packet_count) < config.max_packet_num)
    {
        sleep(1); // 每秒刷新一次

        long wall_now = get_wall_time_us();
        long cpu_now = get_cpu_time_microseconds();

        double wall_ms = (wall_now - wall_start) / 1000.0;
        double cpu_ms = (cpu_now - cpu_start) / 1000.0;
        long sent = atomic_load(&sent_packet_count);

        printf("\033[H\033[J");
        printf("\r 运行时间(ms): %-10.2f     CPU 使用时间(ms):%-10.2f     发送数据包数量：%-10ld",
            wall_ms, cpu_ms, sent);
        fflush(stdout); // 强制刷新输出
    }

    for (int i = 0; i < config.thread_num; ++i)
        pthread_join(threads[i], NULL);

    long wall_now = get_wall_time_us();
    long cpu_now = get_cpu_time_microseconds();
    double wall_ms = (wall_now - wall_start) / 1000.0;
    double cpu_ms = (cpu_now - cpu_start) / 1000.0;
    long sent = atomic_load(&sent_packet_count);

    printf("\033[H\033[J");
    printf("\r 运行时间(ms): %-10.2f     CPU 使用时间(ms):%-10.2f     发送数据包数量：%-10ld\n",
        wall_ms, cpu_ms, sent);
    close(sockfd);
    return 0;
}
