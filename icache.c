#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>

static inline uint64_t ns_now(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec*1000000000ull + (uint64_t)ts.tv_nsec;
}

extern void (*ALL[])(volatile uint64_t *);
extern const size_t ALL_N;

static double bench(void (**fns)(volatile uint64_t*), size_t n, size_t rounds){
    volatile uint64_t sink = 0;

    for (size_t r = 0; r < 3; r++)
        for (size_t i = 0; i < n; i++)
            fns[i](&sink);

    uint64_t t0 = ns_now();
    for (size_t r = 0; r < rounds; r++)
        for (size_t i = 0; i < n; i++)
            fns[i](&sink);
    uint64_t t1 = ns_now();

    if (sink == 0xdeadbeefULL) fprintf(stderr, "keep\n");
    return (double)(t1 - t0) / (double)(n * rounds);
}

int main(void){
    printf("num_funcs,ns_per_call\n");
    for (size_t n = 1; n <= ALL_N; n *= 2) {
        size_t rounds = (n < 32) ? 200000 : (n < 128 ? 120000 : 60000);
        printf("%zu,%.6f\n", n, bench(ALL, n, rounds));
    }
    return 0;
}