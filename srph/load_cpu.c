#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>

#define MATRIX_SIZE 400
#define NUM_THREADS 4  // Adjust this according to the number of cores/threads in your CPU

void multiply_matrices(double A[MATRIX_SIZE][MATRIX_SIZE], double B[MATRIX_SIZE][MATRIX_SIZE], double C[MATRIX_SIZE][MATRIX_SIZE]) {
    for (int i = 0; i < MATRIX_SIZE; ++i) {
        for (int j = 0; j < MATRIX_SIZE; ++j) {
            C[i][j] = 0;
            for (int k = 0; k < MATRIX_SIZE; ++k) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

void* busy_wait(void* arg) {
    double load_time = *((double*)arg);
    struct timeval start, current;
    gettimeofday(&start, NULL);
    double elapsed;
    double A[MATRIX_SIZE][MATRIX_SIZE];
    double B[MATRIX_SIZE][MATRIX_SIZE];
    double C[MATRIX_SIZE][MATRIX_SIZE];

    // Initialize matrices with arbitrary values
    for (int i = 0; i < MATRIX_SIZE; ++i) {
        for (int j = 0; j < MATRIX_SIZE; ++j) {
            A[i][j] = i + j;
            B[i][j] = i - j;
        }
    }

    while (1) {
        gettimeofday(&start, NULL);
        do {
            // Perform matrix multiplication
            multiply_matrices(A, B, C);

            gettimeofday(&current, NULL);
            elapsed = (current.tv_sec - start.tv_sec) + 
                      (current.tv_usec - start.tv_usec) / 1e6;
        } while (elapsed < load_time);
        usleep((1.0 - load_time) * 1e6);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <cpu_load_percent> <duration_seconds>\n", argv[0]);
        return 1;
    }

    int cpu_load_percent = atoi(argv[1]);
    int duration_seconds = atoi(argv[2]);

    if (cpu_load_percent < 0 || cpu_load_percent > 100) {
        fprintf(stderr, "Error: cpu_load_percent must be between 0 and 100.\n");
        return 1;
    }

    double load_time = cpu_load_percent / 100.0;
    pthread_t threads[NUM_THREADS];

    printf("Starting CPU stress test with %d%% load for %d seconds using %d threads...\n", cpu_load_percent, duration_seconds, NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_create(&threads[i], NULL, busy_wait, &load_time);
    }

    sleep(duration_seconds);

    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_cancel(threads[i]);
        pthread_join(threads[i], NULL);
    }

    printf("CPU stress test completed.\n");

    return 0;
}