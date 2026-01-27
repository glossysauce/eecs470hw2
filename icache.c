#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>

static inline uint64_t ns_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

// populate function w a bunch of noops to make it fat 
#define DO_8_NOPS  asm volatile("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
#define DO_64_NOPS DO_8_NOPS DO_8_NOPS DO_8_NOPS DO_8_NOPS DO_8_NOPS DO_8_NOPS DO_8_NOPS DO_8_NOPS
#define DO_512_NOPS DO_64_NOPS DO_64_NOPS DO_64_NOPS DO_64_NOPS DO_64_NOPS DO_64_NOPS DO_64_NOPS DO_64_NOPS

#define MAKE_FUNC(ID) \
__attribute__((noinline)) void f##ID(volatile uint64_t *sink) { \
    DO_512_NOPS \
    *sink += (uint64_t)ID; \
}

MAKE_FUNC(0)  MAKE_FUNC(1)  MAKE_FUNC(2)  MAKE_FUNC(3)
MAKE_FUNC(4)  MAKE_FUNC(5)  MAKE_FUNC(6)  MAKE_FUNC(7)
MAKE_FUNC(8)  MAKE_FUNC(9)  MAKE_FUNC(10) MAKE_FUNC(11)
MAKE_FUNC(12) MAKE_FUNC(13) MAKE_FUNC(14) MAKE_FUNC(15)
MAKE_FUNC(16) MAKE_FUNC(17) MAKE_FUNC(18) MAKE_FUNC(19)
MAKE_FUNC(20) MAKE_FUNC(21) MAKE_FUNC(22) MAKE_FUNC(23)
MAKE_FUNC(24) MAKE_FUNC(25) MAKE_FUNC(26) MAKE_FUNC(27)
MAKE_FUNC(28) MAKE_FUNC(29) MAKE_FUNC(30) MAKE_FUNC(31)


typedef void (*fn_t)(volatile uint64_t*);

static double bench(fn_t *fns, size_t count, size_t rounds) {
    volatile uint64_t sink = 0;

    // warmup
    for (size_t r = 0; r < 2; r++)
        for (size_t i = 0; i < count; i++)
            fns[i](&sink);

    uint64_t t0 = ns_now();
    for (size_t r = 0; r < rounds; r++) {
        for (size_t i = 0; i < count; i++) {
            fns[i](&sink);
        }
    }
    uint64_t t1 = ns_now();

    if (sink == 0xdeadbeefULL) fprintf(stderr, "keep\n");

    double calls = (double)count * (double)rounds;
    return (double)(t1 - t0) / calls;
}

int main(void) {
    fn_t all[] = {
        f0,f1,f2,f3,f4,f5,f6,f7,
        f8,f9,f10,f11,f12,f13,f14,f15,
        f16,f17,f18,f19,f20,f21,f22,f23,
        f24,f25,f26,f27,f28,f29,f30,f31
    };

    printf("num_funcs,ns_per_call\n");
    for (size_t n = 1; n <= sizeof(all)/sizeof(all[0]); n *= 2) {
        size_t rounds = (n < 8) ? 200000 : 80000;
        double ns = bench(all, n, rounds);
        printf("%zu,%.6f\n", n, ns);
    }
    return 0;
}