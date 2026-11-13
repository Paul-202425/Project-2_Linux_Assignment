# Project 2: System calls, I/O and multithreading in C

| Question | Source | What it does |
|---|---|---|
| Q1 | `pipeline.c` | `cat /etc/passwd \| cut -d: -f7 \| sort -u` using fork, pipe, dup2, execvp; saves `result.txt`, prints first 5 lines + line count, reports exit statuses |
| Q2 | `copy_sys.c`, `copy_stdio.c` | File copy with raw syscalls vs stdio, buffer size as argument |
| Q3 | `perfect.c` | Perfect numbers 1..100000, 8 pthreads vs single thread |
| Q4 | `logscan.c` | Thread pool with shared work queue scanning log files for a pattern |

## Build
    make            # gcc -Wall -Wextra -O2 (-pthread for Q3/Q4)

## Run
    ./pipeline
    strace -f -tt -o trace.log ./pipeline
    ./copy_sys big.bin out.bin 4096 ; ./copy_stdio big.bin out.bin 4096
    ./bench_q2.sh                       # 3 runs per buffer size + strace -c + sha256 check
    ./perfect                           # optional arg: upper limit
    ./gen_logs.sh                       # 12 logs, 1-50 MB
    ./logscan ERROR report.txt 4 logs/*.log
    ./bench_q4.sh ERROR                 # 1/2/4/8 threads, 3 runs, speedup, grep check

## Design notes
- Q1: every child closes all six pipe ends after dup2; the parent closes the write end of the last pipe so read() returns 0 (EOF).
- Q3: one mutex guards the shared array (rare, short) and the global counter (locked once per thread, after a local count).
- Q4: separate mutexes for the queue, the report file and the grand totals. Lines longer than 64 KB are counted once, but a pattern straddling a 64 KB chunk boundary would be missed (not an issue for these logs).
- Verification: `sha256sum`/`cmp` for copies, `grep -cF` for logscan (the program uses strstr, i.e. fixed-string matching).
