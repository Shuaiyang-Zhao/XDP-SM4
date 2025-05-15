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
#include <sys/time.h>
#include <signal.h>

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

volatile int keep_running = 1;
void int_handler(int dummy) {
    keep_running = 0;
}

void print_hex(const char *title, const unsigned char *data, size_t length) {
    printf("%s [%zu 字节]: ", title, length);
    for (size_t i = 0; i < length; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 8 == 0) printf(" ");
    }
    printf("\n");
}

double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

void get_cpu_mem_usage(double *cpu, double *mem) {
    static long last_total_user, last_total_user_low, last_total_sys, last_total_idle;
    double percent;

    FILE *fp;
    long user, nice, sys, idle, iowait, irq, softirq, steal;
    fp = fopen("/proc/stat", "r");
    if (!fp) return;

    fscanf(fp, "cpu %ld %ld %ld %ld %ld %ld %ld %ld",
           &user, &nice, &sys, &idle, &iowait, &irq, &softirq, &steal);
    fclose(fp);

    long total_user = user;
    long total_user_low = nice;
    long total_sys = sys;
    long total_idle = idle;

    long total = (total_user - last_total_user) + (total_user_low - last_total_user_low) +
                 (total_sys - last_total_sys);
    long total_all = total + (total_idle - last_total_idle);

    percent = total_all == 0 ? 0 : (total * 100.0 / total_all);
    *cpu = percent;

    last_total_user = total_user;
    last_total_user_low = total_user_low;
    last_total_sys = total_sys;
    last_total_idle = total_idle;

    // 内存使用率
    fp = fopen("/proc/meminfo", "r");
    if (!fp) return;

    long mem_total = 0, mem_available = 0;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %ld kB", &mem_total) == 1) continue;
        if (sscanf(line, "MemAvailable: %ld kB", &mem_available) == 1) break;
    }
    fclose(fp);

    *mem = mem_total == 0 ? 0 : ((mem_total - mem_available) * 100.0 / mem_total);
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
    signal(SIGINT, int_handler);

    int sock_encrypt = bind_socket(LOCAL_PORT_ENCRYPT);
    int sock_decrypt = bind_socket(LOCAL_PORT_DECRYPT);

    struct sockaddr_in b_addr, c_addr;
    memset(&b_addr, 0, sizeof(b_addr));
    b_addr.sin_family = AF_INET;
    b_addr.sin_port = htons(PORT_ENCRYPT);
    inet_pton(AF_INET, ENCRYPT_IP, &b_addr.sin_addr);

    memset(&c_addr, 0, sizeof(c_addr));
    c_addr.sin_family = AF_INET;
    c_addr.sin_port = htons(PORT_DECRYPT);
    inet_pton(AF_INET, DECRYPT_IP, &c_addr.sin_addr);

    unsigned char buf[2048];
    size_t plain_len = sizeof(MESSAGE);
    int round = 0;
    double total_delay = 0, max_delay = 0, min_delay = 1e9;

    while (keep_running) {
        round++;
        printf("\n🔁 第 %d 轮通信开始\n", round);
        print_hex("📤 原始明文", MESSAGE, plain_len);

        double t1 = get_time_ms();
        sendto(sock_encrypt, MESSAGE, plain_len, 0, (struct sockaddr*)&b_addr, sizeof(b_addr));

        double t2, t3;
        ssize_t n;
        t2 = get_time_ms();
        while ((n = recvfrom(sock_encrypt, buf, sizeof(buf), 0, NULL, NULL)) <= 0)
            usleep(1000);
        t3 = get_time_ms();
        print_hex("📥 从 B 收到密文", buf, n);

        double enc_time = t3 - t1;
        double enc_throughput = (n * 8) / (enc_time / 1000.0) / 1e6; // Mbps

        sendto(sock_decrypt, buf, n, 0, (struct sockaddr*)&c_addr, sizeof(c_addr));

        double t4, t5;
        t4 = get_time_ms();
        while ((n = recvfrom(sock_decrypt, buf, sizeof(buf), 0, NULL, NULL)) <= 0)
            usleep(1000);
        t5 = get_time_ms();
        print_hex("📥 解密结果", buf, plain_len);

        double dec_time = t5 - t4;
        double total = t5 - t1;

        if (total > max_delay) max_delay = total;
        if (total < min_delay) min_delay = total;
        total_delay += total;
        double avg_delay = total_delay / round;

        double cpu = 0, mem = 0;
        get_cpu_mem_usage(&cpu, &mem);

        printf("📊 当前轮性能指标：\n");
        printf("加密时间（ms）: %.3f\n", enc_time);
        printf("解密时间（ms）: %.3f\n", dec_time);
        printf("端到端延迟（ms）: %.3f\n", total);
        printf("系统吞吐率（Mbps）: %.3f\n", enc_throughput);
        printf("CPU 使用率（%%）: %.2f\n", cpu);
        printf("内存使用率（%%）: %.2f\n", mem);
        printf("端到端延迟最大值（ms）: %.3f\n", max_delay);
        printf("端到端延迟最小值（ms）: %.3f\n", min_delay);
        printf("端到端延迟平均值（ms）: %.3f\n", avg_delay);

        sleep(1);
    }

    close(sock_encrypt);
    close(sock_decrypt);
    printf("\n🛑 程序终止，完成所有轮次测试。\n");
    return 0;
}

