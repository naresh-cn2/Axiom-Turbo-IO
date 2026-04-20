#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>
#include <pthread.h>
#include <time.h>

#define RING_SIZE 1048576 // 1M slots for stress testing
#define NUM_CONSUMERS 3
#define TOTAL_RECORDS 100000000 // 100M records

typedef struct {
    uint64_t data[RING_SIZE];
    alignas(64) _Atomic uint64_t tail;
    alignas(64) _Atomic uint64_t heads[NUM_CONSUMERS];
} axiom_queue_t;

// PRODUCER: V3.1 Multi-Head Sync
void* producer(void* arg) {
    axiom_queue_t *q = (axiom_queue_t*)arg;
    for (uint64_t i = 0; i < TOTAL_RECORDS; i++) {
        uint64_t current_tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
        uint64_t next_tail = current_tail + 1;

        // Stall until the slowest consumer clears space
        bool full = true;
        while (full) {
            full = false;
            for (int j = 0; j < NUM_CONSUMERS; j++) {
                uint64_t c_head = atomic_load_explicit(&q->heads[j], memory_order_acquire);
                if (next_tail - c_head >= RING_SIZE) {
                    full = true;
                    break;
                }
            }
        }

        q->data[current_tail % RING_SIZE] = i;
        atomic_store_explicit(&q->tail, next_tail, memory_order_release);
    }
    return NULL;
}

// CONSUMER: Independent Progress
typedef struct { axiom_queue_t *q; int id; } thread_arg_t;
void* consumer(void* arg) {
    thread_arg_t *t = (thread_arg_t*)arg;
    uint64_t local_head = 0;
    while (local_head < TOTAL_RECORDS) {
        uint64_t current_tail = atomic_load_explicit(&t->q->tail, memory_order_acquire);
        while (local_head < current_tail) {
            uint64_t val = t->q->data[local_head % RING_SIZE];
            local_head++;
            atomic_store_explicit(&t->q->heads[t->id], local_head, memory_order_release);
        }
    }
    return NULL;
}

int main() {
    axiom_queue_t *q = calloc(1, sizeof(axiom_queue_t));
    pthread_t prod_tid, cons_tid[NUM_CONSUMERS];
    thread_arg_t args[NUM_CONSUMERS];

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pthread_create(&prod_tid, NULL, producer, q);
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        args[i].q = q; args[i].id = i;
        pthread_create(&cons_tid[i], NULL, consumer, &args[i]);
    }

    pthread_join(prod_tid, NULL);
    for (int i = 0; i < NUM_CONSUMERS; i++) pthread_join(cons_tid[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
    printf("Processed %lu records across %d consumers in %.2f seconds.\n", TOTAL_RECORDS, NUM_CONSUMERS, time_taken);
    printf("Throughput: %.2f M records/sec\n", (TOTAL_RECORDS / time_taken) / 1e6);

    free(q);
    return 0;
}
