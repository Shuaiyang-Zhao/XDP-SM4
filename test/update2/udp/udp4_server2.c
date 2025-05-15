#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <pthread.h>
#include <math.h>


#define LISTEN_IP "192.168.58.129"
#define LISTEN_PORT 1233
#define MAX_PACKET_NUM 1000
#define DATA_SIZE 15
#define LOSS_THRESHOLD_NS 50000000  // 50ms
#define MAX_THREADS 10  // 设置最大线程池大小

#define DEBUG 0

typedef struct {
    uint32_t seq;
    uint64_t timestamp;
    unsigned char send_data[DATA_SIZE];
} __attribute__((packed)) packet_t;

typedef struct {
    packet_t pkt;
    void (*callback)(packet_t*);  // callback 接受 packet_t* 类型的参数
} task_t;

pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER;  // 任务队列互斥锁
pthread_cond_t task_cond = PTHREAD_COND_INITIALIZER;  // 任务条件变量
task_t task_queue[MAX_PACKET_NUM];
int task_count = 0;  // 当前任务数
int task_index = 0;  // 任务队列的当前任务索引
int active_threads = 0;  // 当前活动线程数

uint64_t get_timestamp_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts); // 保持一致
    return (uint64_t)(ts.tv_sec * 1000000000ULL + ts.tv_nsec);
}

#ifdef DEBUG
void print_packet(packet_t* pkt, const char* prefix) {
    printf("[%s] 序号: %u, 时间戳: %" PRIu64 ", 数据: ", prefix, pkt->seq, pkt->timestamp);
    for (int i = 0; i < DATA_SIZE; ++i) {
        printf("%02x ", pkt->send_data[i]);
    }
    printf("\n");
}
#endif

// 处理任务的线程
void* thread_worker(void* arg) {
    while (1) {
        pthread_mutex_lock(&task_mutex);  // 锁定任务队列

        // 如果任务队列为空，等待新任务
        while (task_count == 0) {
            pthread_cond_wait(&task_cond, &task_mutex);
        }

        // 获取任务
        task_t task = task_queue[task_index];
        task_index = (task_index + 1) % MAX_PACKET_NUM;
        task_count--;

        pthread_mutex_unlock(&task_mutex);  // 解锁任务队列

        // 执行任务
        task.callback(&task.pkt);  // 传递指针
    }

    return NULL;
}

// 处理每个接收到的数据包的函数
void handle_packet(packet_t* pkt) {  // 修改为接收 packet_t* 类型
    uint64_t now = get_timestamp_ns();
    uint64_t rtt = now >= pkt->timestamp ? (now - pkt->timestamp) : 0;

    // 打印接收到的数据包内容
    

    // 如果延迟大于阈值，丢包
    if (rtt > LOSS_THRESHOLD_NS) {
        // 丢包处理逻辑（如果需要）
    }

    // 打印延迟信息
    if(DEBUG){
    print_packet(pkt, "接收");
    printf("已收序号: %u，接收时间戳：%" PRIu64 ", 延迟: %.2f ms\n", pkt->seq, now, rtt / 1e6);
}
}

int compare(const void* a, const void* b) {
    return (*(double*)a - *(double*)b);
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len = sizeof(cliaddr);
    packet_t pkt;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("创建套接字失败");
        exit(EXIT_FAILURE);
    }

    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(LISTEN_PORT);
    inet_pton(AF_INET, LISTEN_IP, &servaddr.sin_addr);

    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("绑定失败");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    uint32_t count = 0, loss_count = 0;
    uint64_t total_latency = 0, min_latency = UINT64_MAX, max_latency = 0;
    uint64_t start_time = get_timestamp_ns();

    // 为延迟数据分配内存
    double latencies_us[MAX_PACKET_NUM];
    size_t latency_count = 0;
    double sum_squared_latency = 0;

    printf("服务端已启动，监听地址 %s:%d ...\n", LISTEN_IP, LISTEN_PORT);

    // 启动线程池
    pthread_t threads[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, thread_worker, NULL);
    }

    while (count < MAX_PACKET_NUM) {
        ssize_t n = recvfrom(sockfd, &pkt, sizeof(pkt), 0,
                             (struct sockaddr*)&cliaddr, &len);
        if (n < 0) {
            perror("接收失败");
            continue;
        }

        uint64_t now = get_timestamp_ns();
        uint64_t rtt = now >= pkt.timestamp ? (now - pkt.timestamp) : 0;

        if (rtt > LOSS_THRESHOLD_NS) {
            loss_count++;
        }

        total_latency += rtt;
        if (rtt < min_latency) min_latency = rtt;
        if (rtt > max_latency) max_latency = rtt;

        // 记录延迟数据
        latencies_us[latency_count] = rtt / 1000.0;  // 转换为微秒
        sum_squared_latency += latencies_us[latency_count] * latencies_us[latency_count];
        latency_count++;

        // 添加任务到任务队列
        task_t task = {pkt, handle_packet};  // 无需修改，初始化时直接传递回调函数

        pthread_mutex_lock(&task_mutex);  // 锁定任务队列
        if (task_count < MAX_PACKET_NUM) {
            task_queue[(task_index + task_count) % MAX_PACKET_NUM] = task;
            task_count++;
            pthread_cond_signal(&task_cond);  // 通知线程池有任务了
        }
        pthread_mutex_unlock(&task_mutex);  // 解锁任务队列

        ++count;
    }

    uint64_t end_time = get_timestamp_ns();
    double duration_sec = (end_time - start_time) / 1e9;

    // 排序延迟数据用于计算 P95 和 P99
    qsort(latencies_us, latency_count, sizeof(double), compare);

    double p95_latency_us = latencies_us[(int)(latency_count * 0.95)];
    double p99_latency_us = latencies_us[(int)(latency_count * 0.99)];

    // 计算平均延迟
    double average_latency_us = total_latency / (double)count / 1000.0;  // 转换为微秒

    // 计算标准差
    double variance = (sum_squared_latency / count) - (average_latency_us * average_latency_us);
    double std_deviation_us = (variance > 0) ? sqrt(variance) : 0;

    // 计算抖动（标准差的近似值）
    double jitter_us = std_deviation_us;

    printf("\n======= 测试总结 =======\n");
    printf("总接收数据包数  ：%u 个\n", count);
    printf("延迟超 50ms 的丢包：%u 个\n", loss_count);
    printf("丢包率           ：%.2f%%\n", 100.0 * loss_count / count);
    printf("平均延迟         ：%.2f 微秒\n", average_latency_us);
    printf("最小延迟         ：%.2f 微秒\n", min_latency / 1000.0);
    printf("最大延迟         ：%.2f 微秒\n", max_latency / 1000.0);
    printf("P95 延迟         ：%.2f 微秒\n", p95_latency_us);
    printf("P99 延迟         ：%.2f 微秒\n", p99_latency_us);
    printf("标准差           ：%.2f 微秒\n", std_deviation_us);
    printf("抖动             ：%.2f 微秒\n", jitter_us);
    printf("测试总时长       ：%.2f 秒\n", duration_sec);

    close(sockfd);
    return 0;
}
