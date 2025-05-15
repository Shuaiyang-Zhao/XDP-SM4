#include <sys/resource.h>
#include <stdio.h>

void print_resources_usage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf("User CPU time: %ld.%06ld seconds\n", usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
    printf("System CPU time: %ld.%06ld seconds\n", usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
    printf("Maximum resident set size: %ld KB\n", usage.ru_maxrss);
    printf("Page reclaims: %ld\n", usage.ru_minflt);
    printf("Page faults: %ld\n", usage.ru_majflt);
}

int main() {
    // 运行程序的主逻辑
    print_resources_usage();
    return 0;
}
