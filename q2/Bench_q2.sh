#!/bin/bash
# Q2 benchmark: 3 runs per (version, buffer size), average, verify, strace -c
set -e
[ -f big.bin ] || dd if=/dev/urandom of=big.bin bs=1M count=100 status=none
SRC_SUM=$(sha256sum big.bin | cut -d' ' -f1)
printf "%-12s %-8s %-10s %-10s %-10s\n" version buffer avg_time_s read_calls write_calls
for prog in copy_sys copy_stdio; do
  for bs in 512 4096 65536; do
    tot=0
    for r in 1 2 3; do
      t=$(./$prog big.bin out.bin $bs | awk '/Elapsed/{print $3}')
      tot=$(echo "$tot + $t" | bc -l)
    done
    avg=$(echo "scale=6; $tot / 3" | bc -l)
    [ "$(sha256sum out.bin | cut -d' ' -f1)" = "$SRC_SUM" ] || echo "CHECKSUM MISMATCH $prog $bs"
    strace -c -o strace_${prog}_${bs}.txt ./$prog big.bin out.bin $bs > /dev/null
    rd=$(awk '$NF=="read"{print $4}'  strace_${prog}_${bs}.txt)
    wr=$(awk '$NF=="write"{print $4}' strace_${prog}_${bs}.txt)
    printf "%-12s %-8s %-10s %-10s %-10s\n" $prog $bs $avg $rd $wr
  done
done
