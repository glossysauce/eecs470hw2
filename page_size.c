#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

static inline uint64_t ns_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + ts.tv_nsec;
}

int main(void) {
    const size_t MAX_STRIDE = 1 << 20;   // test up to 1 MB
    const size_t ARRAY_SIZE = 64 * 1024 * 1024; // 64 MB
    const int REPS = 2000000;

    uint8_t *arr;
    posix_memalign((void**)&arr, 4096, ARRAY_SIZE);

    printf("stride_bytes,ns_per_access\n");

    for (size_t stride = 64; stride <= MAX_STRIDE; stride *= 2) {
        volatile uint64_t sink = 0;

        uint64_t t0 = ns_now();
        for (int r = 0; r < REPS; r++) {
            size_t idx = (r * stride) & (ARRAY_SIZE - 1);
            sink += arr[idx];
        }
        uint64_t t1 = ns_now();

        double ns = (double)(t1 - t0) / REPS;
        printf("%zu,%.4f\n", stride, ns);

        if (sink == 0xdeadbeefULL) printf("keep\n");
    }

    free(arr);
    return 0;
}