#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"

void get_memory_info(long *total, long *available) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %ld kB", total) == 1) {
            continue;
        }
        if (sscanf(line, "MemAvailable: %ld kB", available) == 1) {
            continue;
        }
    }

    fclose(fp);
}

int main() {
    long total_memory = 0;
    long available_memory = 0;

    while (1) {
        get_memory_info(&total_memory, &available_memory);

        system("clear");

        printf(GREEN "Available Memory: %ld kB\n", available_memory);
        printf(RED "Used Memory: %ld kB\n", total_memory - available_memory);

        sleep(1);
    }

    return 0;
}