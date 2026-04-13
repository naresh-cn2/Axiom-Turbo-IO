#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

int main() {
    const char *filename = "server.log";
    int fd = open(filename, O_RDONLY);
    if (fd == -1) { perror("Error opening file"); return 1; }

    struct stat sb;
    fstat(fd, &sb);
    size_t length = sb.st_size;

    char *addr = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) { perror("mmap failed"); close(fd); return 1; }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int count = 0;
    // THE SHOCK TRIGGER: Look for '"' to identify log fields
    const __m256i target = _mm256_set1_epi8('"'); 

    for (size_t i = 0; i < length - 32; i += 32) {
        __m256i data = _mm256_loadu_si256((__m256i*)&addr[i]);
        __m256i comparison = _mm256_cmpeq_epi8(data, target);
        int mask = _mm256_movemask_epi8(comparison);

        if (mask != 0) {
            for (int bit = 0; bit < 32; bit++) {
                if ((mask >> bit) & 1) {
                    // Pattern scan: Look for " 404 "
                    if (i + bit + 5 < length && 
                        addr[i+bit+1] == ' ' && 
                        addr[i+bit+2] == '4' && 
                        addr[i+bit+3] == '0' && 
                        addr[i+bit+4] == '4') {
                        count++;
                    }
                }
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
    
    printf("--- AXIOM VECTOR-X REPORT ---\n");
    printf("Hardware: Ryzen 7 (AVX2 Vectorized)\n");
    printf("Data Processed: %.2f MB\n", (double)length / (1024 * 1024));
    printf("Throughput: %.2f GB/s\n", ((double)length / 1e9) / time_taken);
    printf("Execution Time: %.6f seconds\n", time_taken);
    printf("Total 404 Errors: %d\n", count);
    printf("-----------------------------\n");

    munmap(addr, length);
    close(fd);
    return 0;
}
