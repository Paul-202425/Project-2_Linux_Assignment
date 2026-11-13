/* Q2 v2: copy with fopen/fread/fwrite/fclose. Usage: ./copy_stdio src dst bufsize */
#include <stdio.h>
#include <stdlib.h>
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

    FILE *in = fopen(argv[1], "rb");
    if (!in) { perror(argv[1]); free(buf); return 1; }
    FILE *out = fopen(argv[2], "wb");
    if (!out) { perror(argv[2]); fclose(in); free(buf); return 1; }

    long freads = 0, fwrites = 0;
    size_t n;
    int rc = 0;
    while ((n = fread(buf, 1, bs, in)) > 0) {
        freads++;
        if (fwrite(buf, 1, n, out) != n) { perror("fwrite"); rc = 1; break; }
        fwrites++;
    }
    if (ferror(in)) { perror("fread"); rc = 1; }
    fclose(in);
    if (fclose(out) != 0) { perror("fclose dst"); rc = 1; }
    free(buf);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double el = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    printf("copy_stdio buffer=%ld fread()=%ld fwrite()=%ld\n", bs, freads, fwrites);
    printf("Elapsed time: %.6f seconds\n", el);
    return rc;
}
