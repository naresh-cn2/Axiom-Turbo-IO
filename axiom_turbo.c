#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>

// Optimized for Ryzen 7 (8 Cores / 16 Logical Threads)
#define NUM_THREADS 16

typedef struct {
    char *map_ptr;
    size_t start_offset;
    size_t end_offset;
    size_t file_size;
    long long count;
} ThreadData;

void* count_lines(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    char *ptr = data->map_ptr;
    size_t current = data->start_offset;
    long long local_count = 0;

    // 1. BOUNDARY HANDLING: If not the first chunk, skip the first partial line
    if (data->start_offset > 0) {
        while (current < data->file_size && ptr[current] != '\n') {
            current++;
        }
        current++; // Move past the newline
    }

    // 2. SIMD ACCELERATED CORE: Use memchr to blast through the middle
    const char *start_ptr = ptr + current;
    const char *end_ptr = ptr + data->end_offset;
    const char *search_ptr = start_ptr;

    while (search_ptr < end_ptr && (search_ptr = memchr(search_ptr, '\n', end_ptr - search_ptr))) {
        local_count++;
        search_ptr++;
    }

    // 3. FINAL VALIDATION: Ensure current is updated for the return logic
    data->count = local_count;
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: ./axiom_turbo <filename>\n");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return 1;
    }

    struct stat st;
    fstat(fd, &st);
    size_t file_size = st.st_size;

    // Memory Mapping the file for zero-copy access
    char *map = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];
    size_t chunk_size = file_size / NUM_THREADS;

    struct timeval start, end;
    gettimeofday(&start, NULL);

    // Spawning Worker Threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].map_ptr = map;
        thread_data[i].file_size = file_size;
        thread_data[i].start_offset = i * chunk_size;
        thread_data[i].end_offset = (i == NUM_THREADS - 1) ? file_size : (i + 1) * chunk_size;
        pthread_create(&threads[i], NULL, count_lines, &thread_data[i]);
    }

    long long total_lines = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        total_lines += thread_data[i].count;
    }

    gettimeofday(&end, NULL);
    double time_spent = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    double throughput = (file_size / (1024.0 * 1024.0 * 1024.0)) / time_spent;

    printf("\n🚀 Axiom TURBO (SIMD-Accelerated Parallel Engine)\n");
    printf("----------------------------------------------\n");
    printf("Total Lines Found: %lld\n", total_lines);
    printf("Execution Time:    %.6f seconds\n", time_spent);
    printf("Throughput:        %.2f GB/s\n", throughput);
    printf("----------------------------------------------\n");

    munmap(map, file_size);
    close(fd);
    return 0;
}