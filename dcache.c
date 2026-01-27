#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static inline uint64_t ns_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static void shuffle_u32(uint32_t *a, size_t n, uint32_t *seed) {
    for (size_t i = n - 1; i > 0; i--) {
        uint32_t x = *seed;
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        *seed = x;

        size_t j = (size_t)(x % (i + 1));
        uint32_t tmp = a[i];
        a[i] = a[j];
        a[j] = tmp;
    }
}

static double bench_pointer_chase(uint32_t *next, size_t steps) {
    volatile uint32_t idx = 0;

    //warm-up
    for (size_t i = 0; i < steps; i++) idx = next[idx];

    uint64_t t0 = ns_now();
    for (size_t i = 0; i < steps; i++) idx = next[idx];
    uint64_t t1 = ns_now();

    if (idx == 0xdeadbeef) fprintf(stderr, "keep\n");

    return (double)(t1 - t0) / (double)steps;
}

int main(void) {
    const size_t line = 64;
    const size_t min_bytes = 4 * 1024;
    const size_t max_bytes = 64 * 1024 * 1024;

    printf("working_set_bytes,ns_per_access\n");

    uint32_t seed = 0x12345678u;

    for (size_t bytes = min_bytes; bytes <= max_bytes; bytes *= 2) {
        size_t nodes = bytes / line;
        if (nodes < 2) nodes = 2;

        uint32_t *next = NULL;
        if (posix_memalign((void**)&next, 64, nodes * sizeof(uint32_t)) != 0) {
            perror("posix_memalign");
            return 1;
        }
        uint32_t *perm = NULL;
        if (posix_memalign((void**)&perm, 64, nodes * sizeof(uint32_t)) != 0) {
            perror("posix_memalign perm");
            return 1;
        }
        for (size_t i = 0; i < nodes; i++) perm[i] = (uint32_t)i;
        shuffle_u32(perm, nodes, &seed);
        for (size_t i = 0; i + 1 < nodes; i++) next[perm[i]] = perm[i + 1];
        next[perm[nodes - 1]] = perm[0];

        size_t steps = nodes * 200;
        if (steps < 5 * 1000 * 1000) steps = 5 * 1000 * 1000;

        double ns = bench_pointer_chase(next, steps);
        printf("%zu,%.6f\n", bytes, ns);

        free(perm);
        free(next);
    }

    return 0;
}