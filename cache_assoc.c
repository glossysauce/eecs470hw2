#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static inline uint64_t ns_now(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec*1000000000ull + (uint64_t)ts.tv_nsec;
}

int main(void){
    long l1 = 32
    line = -1;

#ifdef _SC_LEVEL1_DCACHE_SIZE
    l1 = sysconf(_SC_LEVEL1_DCACHE_SIZE);
#endif
#ifdef _SC_LEVEL1_DCACHE_LINESIZE
    line = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
#endif
    if (l1 <= 0) l1 = 32*1024;
    if (line <= 0) line = 64;

    const int KMAX = 32;
    const int TRIALS = 200000;
    size_t stride = (size_t)l1;                 // try to hit same L1 set each step
    size_t bytes  = stride * (KMAX + 2) + 4096;

    uint8_t *buf;
    if (posix_memalign((void**)&buf, 4096, bytes) != 0) return 1;

    // Build addresses: base + i*stride
    volatile uint8_t *a[KMAX+1];
    uintptr_t base = ((uintptr_t)buf + (line-1)) & ~(uintptr_t)(line-1);
    for (int i=0;i<=KMAX;i++) a[i] = (volatile uint8_t*)(base + (uintptr_t)i*stride);

    printf("assume stride=%zu (L1=%ldB), line=%ldB\n", stride, l1, line);
    printf("k, ns_per_probe\n");

    for (int k=1;k<=KMAX;k++){
        volatile uint64_t sink = 0;

        // Warm pages
        for (int i=0;i<10000;i++) sink += a[i % (k+1)][0];

        uint64_t t0 = ns_now();
        for (int t=0;t<TRIALS;t++){
            // Touch k conflicting lines
            for (int i=1;i<=k;i++) sink += a[i][0];

            // Probe line 0 after conflicts
            sink += a[0][0];
        }
        uint64_t t1 = ns_now();

        double ns = (double)(t1 - t0) / (double)TRIALS;
        printf("%d, %.6f\n", k, ns);

        if (sink == 0xdeadbeefULL) fprintf(stderr,"keep\n");
    }

    free(buf);
    return 0;
}