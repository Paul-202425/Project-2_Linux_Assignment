/* Q3: perfect numbers, 8 POSIX threads vs single thread.
   Usage: ./perfect [limit]   (default limit = 100000) */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define NTHREADS 8
#define MAXPERF 64

typedef struct { int id; long start, end; } seg_t;

static pthread_mutex_t lock;
static long global_count = 0;
static long found[MAXPERF];
static int  found_n = 0;

static int is_perfect(long n)
{
    if (n < 2) return 0;
    long sum = 1;                                  /* 1 divides everything */
    for (long i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            sum += i;
            if (i != n / i) sum += n / i;
        }
    }
    return sum == n;
}

static void *worker(void *arg)
{
    seg_t *s = arg;
    long local = 0;
    for (long n = s->start; n <= s->end; n++) {
        if (is_perfect(n)) {
            local++;
            pthread_mutex_lock(&lock);             /* shared array: rare, short */
            if (found_n < MAXPERF) found[found_n++] = n;
            pthread_mutex_unlock(&lock);
        }
    }
    pthread_mutex_lock(&lock);                     /* shared counter: once per thread */
    global_count += local;
    pthread_mutex_unlock(&lock);
    return NULL;
}

static int cmp(const void *a, const void *b)
{
    long x = *(const long *)a, y = *(const long *)b;
    return (x > y) - (x < y);
}

static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

int main(int argc, char *argv[])
{
    long limit = (argc > 1) ? atol(argv[1]) : 100000;
    long seg = limit / NTHREADS;                   /* 12,500 for 100,000 */

    /* ---- single-threaded ---- */
    double t0 = now();
    long single_count = 0;
    for (long n = 1; n <= limit; n++) if (is_perfect(n)) single_count++;
    double single_t = now() - t0;

    /* ---- multi-threaded ---- */
    pthread_t th[NTHREADS];
    seg_t segs[NTHREADS];
    if (pthread_mutex_init(&lock, NULL) != 0) { fprintf(stderr, "mutex init failed\n"); return 1; }

    t0 = now();
    for (int i = 0; i < NTHREADS; i++) {
        segs[i].id = i;
        segs[i].start = i * seg + 1;
        segs[i].end = (i == NTHREADS - 1) ? limit : (i + 1) * seg;
        if (pthread_create(&th[i], NULL, worker, &segs[i]) != 0) {
            fprintf(stderr, "pthread_create failed\n"); return 1;
        }
    }
    for (int i = 0; i < NTHREADS; i++) pthread_join(th[i], NULL);
    double multi_t = now() - t0;
    pthread_mutex_destroy(&lock);

    qsort(found, found_n, sizeof found[0], cmp);
    printf("Perfect numbers found:");
    for (int i = 0; i < found_n; i++) printf(" %ld", found[i]);
    printf("\n");
    printf("The synchronized total number of perfect numbers between 1 and %ld is %ld\n",
           limit, global_count);
    if (single_count != global_count)
        fprintf(stderr, "WARNING: single (%ld) and multi (%ld) counts differ\n", single_count, global_count);
    printf("Single-threaded time: %.6f seconds\n", single_t);
    printf("Multi-threaded time:  %.6f seconds\n", multi_t);
    return 0;
}
