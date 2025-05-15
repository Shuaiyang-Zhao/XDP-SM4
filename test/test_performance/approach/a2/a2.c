//性能指标：吞吐量、丢包率、平均延迟、最小/最大延迟、标准差、P95 / P99 延迟、抖动	、CPU 使用时间、内存使用量、网络接收增量、网络发送增量,结果写入CSV

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/statvfs.h>
#include <sys/resource.h>
#include <math.h>

#define SERVER_IP "192.168.58.128"
#define SERVER_PORT 1234

#define NUM_TESTS 1000// 发包数量
#define NUM_GROUPS 10 // 测试轮数

#define DATA_SIZE 15
char send_data[DATA_SIZE] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32};


typedef struct
{
    long throughput;
    double packet_loss;
    double avg_latency;
    long min_latency;
    long max_latency;
    double stddev_latency;
    double jitter;
    long cpu_time;
    long mem_usage;
    unsigned long rx_delta;
    unsigned long tx_delta;
} TestResult;

// 启用调试模式，设置为 1 启用调试输出，0 则禁用
#define DEBUG 0

#if DEBUG
#define DEBUG_PRINT(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) // 无输出
#endif

// 打印数据的十六进制表示
void print_hex(const char *data, int len)
{
    for (int i = 0; i < len; i++)
    {
        printf("%02x ", (unsigned char)data[i]);
    }
    printf("\n");
}

// 获取进程 CPU 使用时间（用户态 + 内核态，单位：微秒）
long get_cpu_time_microseconds()
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_utime.tv_sec * 1000000L + usage.ru_utime.tv_usec +
           usage.ru_stime.tv_sec * 1000000L + usage.ru_stime.tv_usec;
}

// 读取 /proc/net/dev 获取指定网卡的数据（如 "ens33"），返回接收与发送字节数
int get_network_usage(const char *iface, unsigned long *rx_bytes, unsigned long *tx_bytes)
{
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp)
    {
        perror("无法打开 /proc/net/dev");
        return -1;
    }
    char buffer[256];
    // 略过前两行表头
    fgets(buffer, sizeof(buffer), fp);
    fgets(buffer, sizeof(buffer), fp);
    int found = 0;
    while (fgets(buffer, sizeof(buffer), fp))
    {
        if (strstr(buffer, iface))
        {
            char dummy[32];
            unsigned long r_bytes;
            // 解析冒号前的数据以及冒号后的第一个字段（接收字节数）
            if (sscanf(buffer, "%[^:]: %lu", dummy, &r_bytes) == 2)
            {
                char *p = strchr(buffer, ':');
                if (p != NULL)
                {
                    p++; // 跳过冒号
                    // 连续调用 strtok 分割，取第 9 个字段（发送字节数）
                    char *token = strtok(p, " ");
                    for (int i = 1; i < 9 && token != NULL; i++)
                    {
                        token = strtok(NULL, " ");
                    }
                    if (token)
                    {
                        *rx_bytes = r_bytes;
                        *tx_bytes = strtoul(token, NULL, 10);
                        found = 1;
                        break;
                    }
                }
            }
        }
    }
    fclose(fp);
    return found ? 0 : -1;
}

// 读取当前进程内存使用量 (VmRSS)，单位为 KB
long get_memory_usage_kb()
{
    FILE *fp = fopen("/proc/self/status", "r");
    if (fp == NULL)
    {
        perror("无法打开 /proc/self/status");
        return -1;
    }
    char line[256];
    long mem_usage = -1;
    while (fgets(line, sizeof(line), fp))
    {
        if (strncmp(line, "VmRSS:", 6) == 0)
        {
            sscanf(line + 6, "%ld", &mem_usage);
            break;
        }
    }
    fclose(fp);
    return mem_usage;
}

// 将所有测试结果一次性写入 CSV 文件（避免重复写入）
// CSV 文件中，前两列为包负载长度和发包数量，其后依次记录轮次和各项指标
void save_results_to_csv(const char *filename, TestResult results[], int count)
{
    int file_exists = access(filename, F_OK) == 0;
    FILE *fp = fopen(filename, "a");
    if (fp == NULL)
    {
        perror("打开文件失败");
        exit(EXIT_FAILURE);
    }

    if (!file_exists)
    {
        // 写入 CSV 表头：包负载长度, 发包数量, 轮次, 吞吐量, 丢包率, 平均延迟, 最小延迟, 最大延迟, 标准差, 抖动, CPU时间, 内存使用, 接收字节, 发送字节
        fprintf(fp, "包负载长度,发包数量,轮次,吞吐量,丢包率,平均延迟,最小延迟,最大延迟,标准差,抖动,CPU时间,内存使用,接收字节,发送字节\n");
    
    }
        
    for (int i = 0; i < count; i++)
    {
        fprintf(fp, "%d,%d,第 %d 轮,%ld,%.2f,%.2f,%ld,%ld,%.2f,%.2f,%ld,%ld,%lu,%lu\n",
                DATA_SIZE, NUM_TESTS, i + 1, results[i].throughput, results[i].packet_loss, results[i].avg_latency,
                results[i].min_latency, results[i].max_latency, results[i].stddev_latency,
                results[i].jitter, results[i].cpu_time, results[i].mem_usage,
                results[i].rx_delta, results[i].tx_delta);
    }
    fclose(fp);
}

// 计算均值并保存到 CSV 文件
// CSV 文件中，前两列为包负载长度和发包数量，其后依次记录各指标均值
void save_average_to_csv(const char *filename, TestResult results[], int count)
{
    long total_throughput = 0;
    double total_packet_loss = 0;
    double total_avg_latency = 0;
    long total_min_latency = 0;
    long total_max_latency = 0;
    double total_stddev_latency = 0;
    double total_jitter = 0;
    long total_cpu_time = 0;
    long total_mem_usage = 0;
    unsigned long total_rx_delta = 0;
    unsigned long total_tx_delta = 0;

    for (int i = 0; i < count; i++)
    {
        total_throughput += results[i].throughput;
        total_packet_loss += results[i].packet_loss;
        total_avg_latency += results[i].avg_latency;
        total_min_latency += results[i].min_latency;
        total_max_latency += results[i].max_latency;
        total_stddev_latency += results[i].stddev_latency;
        total_jitter += results[i].jitter;
        total_cpu_time += results[i].cpu_time;
        total_mem_usage += results[i].mem_usage;
        total_rx_delta += results[i].rx_delta;
        total_tx_delta += results[i].tx_delta;
    }

    DEBUG_PRINT("===== 所有轮次平均结果 =====\n", 0);
    DEBUG_PRINT("平均吞吐量: %ld 字节/秒\n", total_throughput / NUM_GROUPS);
    DEBUG_PRINT("平均丢包率: %.2f%%\n", total_packet_loss / NUM_GROUPS);
    DEBUG_PRINT("平均延迟: %.2f 微秒\n", total_avg_latency / NUM_GROUPS);
    DEBUG_PRINT("平均最小延迟: %ld 微秒\n", total_min_latency / NUM_GROUPS);
    DEBUG_PRINT("平均最大延迟: %ld 微秒\n", total_max_latency / NUM_GROUPS);
    DEBUG_PRINT("平均标准差: %.2f 微秒\n", total_stddev_latency / NUM_GROUPS);
    DEBUG_PRINT("平均抖动: %.2f 微秒\n", total_jitter / NUM_GROUPS);
    DEBUG_PRINT("平均CPU时间: %ld 微秒\n", total_cpu_time / NUM_GROUPS);
    DEBUG_PRINT("平均内存使用: %ld KB\n", total_mem_usage / NUM_GROUPS);
    DEBUG_PRINT("平均接收字节: %lu\n", total_rx_delta / NUM_GROUPS);
    DEBUG_PRINT("平均发送字节: %lu\n", total_tx_delta / NUM_GROUPS);

    FILE *fp = fopen(filename, "a");
    if (fp == NULL)
    {
        perror("打开文件失败");
        exit(EXIT_FAILURE);
    }

    fseek(fp, 0, SEEK_END);
    if (ftell(fp) == 0)
    {
        // 写入表头：包负载长度, 发包数量, 平均吞吐量, 平均丢包率, 平均延迟, 平均最小延迟, 平均最大延迟, 平均标准差, 平均抖动, 平均CPU时间, 平均内存使用, 平均接收字节, 平均发送字节
        fprintf(fp, "包负载长度,发包数量,平均吞吐量,平均丢包率,平均延迟,平均最小延迟,平均最大延迟,平均标准差,平均抖动,平均CPU时间,平均内存使用,平均接收字节,平均发送字节\n");
    }
    
    fprintf(fp, "%d,%d,%ld,%.2f,%.2f,%ld,%ld,%.2f,%.2f,%ld,%ld,%lu,%lu\n",
            DATA_SIZE, NUM_TESTS,
            total_throughput / count, total_packet_loss / count, total_avg_latency / count,
            total_min_latency / count, total_max_latency / count, total_stddev_latency / count,
            total_jitter / count, total_cpu_time / count, total_mem_usage / count,
            total_rx_delta / count, total_tx_delta / count);

    fclose(fp);
}

// 比较函数用于 qsort
int compare(const void *a, const void *b)
{
    long l1 = *(long *)a;
    long l2 = *(long *)b;
    return (l1 > l2) - (l1 < l2);
}

void send_and_receive(int sockfd, struct sockaddr_in *server_addr, TestResult *result)
{

    char recv_data[DATA_SIZE + 1];
    struct timeval start, end;
    long total_latency = 0, min_latency = 1000000, max_latency = 0;
    int sent_count = 0, recv_count = 0;
    long latencies[NUM_TESTS];

    // 测试开始前读取 CPU、网络和内存数据
    long cpu_before = get_cpu_time_microseconds();
    unsigned long rx_before = 0, tx_before = 0;
    if (get_network_usage("ens33", &rx_before, &tx_before) != 0)
    {
        printf("无法获取网络数据，请检查网卡名称或权限。\n");
    }
    long mem_before = get_memory_usage_kb();

    struct timeval test_start;
    gettimeofday(&test_start, NULL);

    for (int i = 0; i < NUM_TESTS; i++)
    {
        gettimeofday(&start, NULL);
        sendto(sockfd, send_data, DATA_SIZE, 0, (struct sockaddr *)server_addr, sizeof(*server_addr));
        sent_count++;

        if (DEBUG && i < NUM_TESTS)
        {
            DEBUG_PRINT("[发送] 数据包 %d: ", i + 1);
            print_hex(send_data, DATA_SIZE);
        }

        int len = recvfrom(sockfd, recv_data, sizeof(recv_data), 0, NULL, NULL);
        gettimeofday(&end, NULL);

        long elapsed_time = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_usec - start.tv_usec);
        total_latency += elapsed_time;
        latencies[i] = elapsed_time;
        if (elapsed_time < min_latency)
            min_latency = elapsed_time;
        if (elapsed_time > max_latency)
            max_latency = elapsed_time;

        if (len > 0)
        {
            recv_count++;
            if (DEBUG && i < NUM_TESTS)
            {
                DEBUG_PRINT("[接收] 数据包 %d: ", i + 1);
                print_hex(recv_data, len);
                DEBUG_PRINT("延迟: %ld 微秒\n", elapsed_time);
            }
        }
    }

    struct timeval test_end;
    gettimeofday(&test_end, NULL);
    long total_time = (test_end.tv_sec - test_start.tv_sec) * 1000000L + (test_end.tv_usec - test_start.tv_usec);

    // 计算吞吐量（字节/秒）
    double throughput = (sent_count * DATA_SIZE) / (total_time / 1000000.0);
    DEBUG_PRINT("吞吐量: %.2f 字节/秒\n", throughput);

    // 计算丢包率
    double packet_loss = (double)(sent_count - recv_count) / sent_count * 100;
    DEBUG_PRINT("丢包率: %.2f%%\n", packet_loss);

    // 延迟统计
    double avg_latency = (double)total_latency / NUM_TESTS;
    DEBUG_PRINT("平均延迟: %.2f 微秒\n", avg_latency);
    DEBUG_PRINT("最小延迟: %ld 微秒\n", min_latency);
    DEBUG_PRINT("最大延迟: %ld 微秒\n", max_latency);

    double variance = 0;
    for (int i = 0; i < NUM_TESTS; i++)
    {
        variance += pow(latencies[i] - avg_latency, 2);
    }
    variance /= NUM_TESTS;
    double stddev_latency = sqrt(variance);
    DEBUG_PRINT("标准差: %.2f 微秒\n", stddev_latency);

    // 计算 P95 和 P99 延迟
    qsort(latencies, NUM_TESTS, sizeof(long), compare);
    double p95 = latencies[(int)(NUM_TESTS * 0.95)];
    double p99 = latencies[(int)(NUM_TESTS * 0.99)];
    DEBUG_PRINT("P95 延迟: %.2f 微秒\n", p95);
    DEBUG_PRINT("P99 延迟: %.2f 微秒\n", p99);

    // 计算抖动：连续数据包延迟差值的平均绝对值
    double total_jitter = 0;
    for (int i = 1; i < NUM_TESTS; i++)
    {
        total_jitter += fabs(latencies[i] - latencies[i - 1]);
    }
    double jitter = total_jitter / (NUM_TESTS - 1);
    DEBUG_PRINT("抖动: %.2f 微秒\n", jitter);

    // 测试结束时读取 CPU、网络和内存数据
    long cpu_after = get_cpu_time_microseconds();
    unsigned long rx_after = 0, tx_after = 0;
    if (get_network_usage("ens33", &rx_after, &tx_after) != 0)
    {
        printf("无法获取网络数据，请检查网卡名称或权限。\n");
    }
    long mem_after = get_memory_usage_kb();

    long cpu_delta = cpu_after - cpu_before;
    unsigned long rx_delta = (rx_after >= rx_before) ? (rx_after - rx_before) : 0;
    unsigned long tx_delta = (tx_after >= tx_before) ? (tx_after - tx_before) : 0;

    result->throughput = throughput;
    result->packet_loss = packet_loss;
    result->avg_latency = avg_latency;
    result->min_latency = min_latency;
    result->max_latency = max_latency;
    result->stddev_latency = stddev_latency;
    result->jitter = jitter;
    result->cpu_time = cpu_delta;
    result->mem_usage = mem_after - mem_before;
    result->rx_delta = rx_delta;
    result->tx_delta = tx_delta;
}

int main()
{
    int sockfd;
    struct sockaddr_in server_addr;
    TestResult results[NUM_GROUPS];

    // 创建 UDP 套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("创建 socket 失败");
        exit(EXIT_FAILURE);
    }

     // 设置接收超时时间为 1 秒
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 50000;  // 50 毫秒 = 50000 微秒

     if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
         perror("设置接收超时失败");
         exit(EXIT_FAILURE);
     }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // 按轮次进行测试并将结果存储到数组中
    for (int i = 0; i < NUM_GROUPS; i++)
    {
        // 输出当前轮次调试信息
        DEBUG_PRINT("===== 开始第 %d 轮测试 =====\n", i + 1);
        send_and_receive(sockfd, &server_addr, &results[i]);
    }

    // 测试完成后，一次性写入所有测试轮次数据到 CSV 文件
    save_results_to_csv("测试结果.csv", results, NUM_GROUPS);
    // 保存各轮测试的均值到另外一个 CSV 文件
    save_average_to_csv("平均结果.csv", results, NUM_GROUPS);

    close(sockfd);
    return 0;
}
