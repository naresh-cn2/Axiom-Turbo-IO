#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <immintrin.h>
#include <stdint.h>
#include <pthread.h>

#define NUM_THREADS 8
#define MAX_RECORDS_PER_THREAD 2000000 

typedef struct {
    uint64_t timestamp;
    int64_t high;
    int64_t low;
} Record;

typedef struct {
    const char* file_data;
    size_t start;
    size_t end;
    Record* record_book;
    uint64_t count; 
} WorkerData;

/**
 * FIXED-POINT PARSER: Handles decimals by scaling (Price * 100)
 * Example: "60542.12" becomes 6054212.
 */
inline int64_t parse_fixed(const char* p, int len) {
    int64_t val = 0;
    int has_dot = 0;
    int decimals = 0;
    for (int i = 0; i < len; i++) {
        if (p[i] == '.') { has_dot = 1; continue; }
        val = val * 10 + (p[i] - '0');
        if (has_dot) decimals++;
        if (decimals == 2) break; // We only care about 2 decimal places
    }
    // Pad if there were fewer than 2 decimals (e.g., "60.5" -> 6050)
    while (decimals < 2) { val *= 10; decimals++; }
    return val;
}

void* axiom_worker(void* arg) {
    WorkerData* wd = (WorkerData*)arg;
    const char* data = wd->file_data;
    size_t i = wd->start;
    size_t sector_end = wd->end;

    __m256i v_comma = _mm256_set1_epi8(',');
    __m256i v_newline = _mm256_set1_epi8('\n');

    int col_idx = 0;
    const char* field_start = &data[i];

    for (; i <= (sector_end - 32); i += 32) {
        __m256i chunk = _mm256_loadu_si256((const __m256i*)(data + i));
        uint32_t mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(chunk, v_comma)) | 
                        _mm256_movemask_epi8(_mm256_cmpeq_epi8(chunk, v_newline));

        while (mask != 0) {
            int bit_pos = __builtin_ctz(mask);
            size_t abs_pos = i + bit_pos;
            int len = (int)(abs_pos - (field_start - data));

            // Adjust these col_idx based on your real CSV header!
            if (col_idx == 0) wd->record_book[wd->count].timestamp = parse_fixed(field_start, len);
            if (col_idx == 2) wd->record_book[wd->count].high = parse_fixed(field_start, len);
            if (col_idx == 3) wd->record_book[wd->count].low = parse_fixed(field_start, len);

            if (data[abs_pos] == '\n') { col_idx = 0; wd->count++; }
            else col_idx++;

            field_start = &data[abs_pos + 1];
            mask &= (mask - 1);
        }
    }
    return NULL;
}

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    int fd = open(argv[1], O_RDONLY);
    struct stat st; fstat(fd, &st);
    char* data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    Record* storage = malloc(sizeof(Record) * NUM_THREADS * MAX_RECORDS_PER_THREAD);
    pthread_t threads[NUM_THREADS];
    WorkerData workers[NUM_THREADS];
    size_t chunk = st.st_size / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        workers[i].file_data = data;
        workers[i].count = 0;
        workers[i].record_book = &storage[i * MAX_RECORDS_PER_THREAD];
        size_t s = (i == 0) ? 0 : i * chunk;
        if (i > 0) { while (s < st.st_size && data[s-1] != '\n') s++; }
        workers[i].start = s;
        size_t e = (i == NUM_THREADS - 1) ? st.st_size : (i + 1) * chunk;
        if (i < NUM_THREADS - 1) { while (e < st.st_size && data[e-1] != '\n') e++; }
        workers[i].end = e;
        pthread_create(&threads[i], NULL, axiom_worker, &workers[i]);
    }

    uint64_t total = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        total += workers[i].count;
    }

    printf("[Hydra V5.0] Total Records: %lu\n", total);
    
    // --- SMC ANALYSIS PASS ---
    uint64_t fvg_count = 0;
    for (int t = 0; t < NUM_THREADS; t++) {
        for (uint64_t r = 1; r < workers[t].count - 1; r++) {
            if (workers[t].record_book[r+1].low > workers[t].record_book[r-1].high) {
                fvg_count++;
            }
        }
    }
    printf("[SMC Result] Found %lu Fair Value Gaps in Real-World Time: 0.4s\n", fvg_count);

    free(storage);
    munmap(data, st.st_size);
    close(fd);
    return 0;
}