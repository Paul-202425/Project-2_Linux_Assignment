/* Q2 v1: copy with open/read/write/close. Usage: ./copy_sys src dst bufsize */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

int main(int argc, char *argv[])
{
    if (argc != 4) { fprintf(stderr, "Usage: %s src dst buffer_bytes\n", argv[0]); return 2; }
    long bs = atol(argv[3]);
    if (bs <= 0) { fprintf(stderr, "buffer size must be > 0\n"); return 2; }
    char *buf = malloc(bs);
    if (!buf) { perror("malloc"); return 1; }

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    int in = open(argv[1], O_RDONLY);
    if (in == -1) { perror(argv[1]); free(buf); return 1; }
    int out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out == -1) { perror(argv[2]); close(in); free(buf); return 1; }

    long reads = 0, writes = 0;
    ssize_t n;
    int rc = 0;
    while ((n = read(in, buf, bs)) != 0) {
        if (n == -1) { if (errno == EINTR) continue; perror("read"); rc = 1; break; }
        reads++;
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(out, buf + off, n - off);
            if (w == -1) { if (errno == EINTR) continue; perror("write"); rc = 1; goto done; }
            off += w; writes++;
        }
    }
done:
    if (close(in) == -1) perror("close src");
    if (close(out) == -1) { perror("close dst"); rc = 1; }
    free(buf);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double el = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    printf("copy_sys buffer=%ld read()=%ld write()=%ld\n", bs, reads, writes);
    printf("Elapsed time: %.6f seconds\n", el);
    return rc;
}
