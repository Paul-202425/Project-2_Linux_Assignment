/* Q4: thread-pool log scanner.
   Usage: ./logscan pattern report.txt <threads> file1.log file2.log ... */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define LINEBUF 65536

static char **queue;            /* shared work queue of file names */
static int qcount, qnext;
static pthread_mutex_t queue_mtx, report_mtx, total_mtx;
static FILE *report;
static const char *pattern;
static long grand_match = 0, grand_files = 0;

static void *worker(void *arg)
{
    int id = (int)(long)arg;
    char *buf = malloc(LINEBUF);
    if (!buf) { perror("malloc"); return NULL; }

    for (;;) {
        pthread_mutex_lock(&queue_mtx);
        int idx = (qnext < qcount) ? qnext++ : -1;
        pthread_mutex_unlock(&queue_mtx);
        if (idx < 0) break;                         /* queue empty */

        const char *name = queue[idx];
        FILE *fp = fopen(name, "r");
        if (!fp) { perror(name); continue; }

        long match = 0, total = 0;
        int in_line = 0, matched = 0;
        while (fgets(buf, LINEBUF, fp)) {
            size_t len = strlen(buf);
            if (!in_line) { in_line = 1; matched = 0; }
            if (!matched && strstr(buf, pattern)) matched = 1;
            if (len > 0 && buf[len - 1] == '\n') {  /* line complete */
                total++;
                if (matched) match++;
                in_line = 0;
            }
        }
        if (in_line) { total++; if (matched) match++; }   /* last line, no newline */
        if (ferror(fp)) perror(name);
        fclose(fp);

        pthread_mutex_lock(&report_mtx);
        fprintf(report, "%s: %ld matching lines out of %ld (thread %d)\n", name, match, total, id);
        fflush(report);
        pthread_mutex_unlock(&report_mtx);

        pthread_mutex_lock(&total_mtx);
        grand_match += match;
        grand_files++;
        pthread_mutex_unlock(&total_mtx);
    }
    free(buf);
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 5) {
        fprintf(stderr, "Usage: %s pattern report.txt <threads> file1.log [file2.log ...]\n", argv[0]);
        return 2;
    }
    pattern = argv[1];
    int nthreads = atoi(argv[3]);
    if (nthreads < 1) { fprintf(stderr, "threads must be >= 1\n"); return 2; }
    queue = &argv[4];
    qcount = argc - 4;
    qnext = 0;

    report = fopen(argv[2], "w");
    if (!report) { perror(argv[2]); return 1; }

    pthread_mutex_init(&queue_mtx, NULL);
    pthread_mutex_init(&report_mtx, NULL);
    pthread_mutex_init(&total_mtx, NULL);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    pthread_t *th = malloc(nthreads * sizeof *th);
    if (!th) { perror("malloc"); return 1; }
    for (long i = 0; i < nthreads; i++)
        if (pthread_create(&th[i], NULL, worker, (void *)(i + 1)) != 0) {
            fprintf(stderr, "pthread_create failed\n"); return 1;
        }
    for (int i = 0; i < nthreads; i++) pthread_join(th[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double el = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

    fprintf(report, "Total matching lines across %ld files: %ld\n", grand_files, grand_match);
    fclose(report);
    printf("Total matching lines across %ld files: %ld\n", grand_files, grand_match);
    printf("Elapsed time: %.6f seconds (threads=%d)\n", el, nthreads);

    pthread_mutex_destroy(&queue_mtx);
    pthread_mutex_destroy(&report_mtx);
    pthread_mutex_destroy(&total_mtx);
    free(th);
    return 0;
}
