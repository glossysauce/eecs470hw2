#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

static inline uint64_t ns_now(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec*1000000000ull + (uint64_t)ts.tv_nsec;
}

static inline void barrier(void){
    asm volatile("" ::: "memory");
}

int main(void){
    const size_t LINE = 64;
    const size_t L1_BYTES = 64 * 2048;  
    const size_t STRIDE = L1_BYTES; 

    const int KMAX = 32; //32ways
    const int WARM = 20000;
    const int TRIALS = 300000;

    size_t bytes = STRIDE * (KMAX + 2) + 4096;
    uint8_t *buf = NULL;
    if (posix_memalign((void**)&buf, 4096, bytes) != 0 || !buf){
        perror("posix_memalign");
        return 1;
    }

    uintptr_t base = ((uintptr_t)buf + (LINE-1)) & ~(uintptr_t)(LINE-1);

    volatile uint8_t *a[KMAX+1];
    for (int i = 0; i <= KMAX; i++){
        a[i] = (volatile uint8_t*)(base + (uintptr_t)i * (uintptr_t)STRIDE);
    }

    printf("stride=%zu bytes (L1=%zu), line=%zu\n", STRIDE, L1_BYTES, LINE);
    printf("k_conflicts, ns_per_probe\n");

    for (int k = 1; k <= KMAX; k++){
        volatile uint64_t sink = 0;

        // warmup
        for (int t = 0; t < WARM; t++){
            sink += a[(t % k) + 1][0];
            sink += a[0][0];
        }

        uint64_t t0 = ns_now();
        for (int t = 0; t < TRIALS; t++){
            for (int i = 1; i <= k; i++){
                sink += a[i][0];
            }
            barrier();
            sink += a[0][0];
        }
        uint64_t t1 = ns_now();

        double ns = (double)(t1 - t0) / (double)TRIALS;
        printf("%d, %.6f\n", k, ns);

        if (sink == 0xdeadbeefULL) fprintf(stderr, "keep\n");
    }

    free(buf);
    return 0;
}