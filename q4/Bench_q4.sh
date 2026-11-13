#!/bin/bash
# Q4 benchmark: 1,2,4,8 threads, 3 runs each, average time and speedup; verify with grep
PAT=${1:-ERROR}
FILES=(logs/*.log)
T1=0
printf "%-8s %-12s %-8s\n" threads avg_time_s speedup
for t in 1 2 4 8; do
  tot=0
  for r in 1 2 3; do
    s=$(./logscan "$PAT" report_$t.txt $t "${FILES[@]}" | awk '/Elapsed/{print $3}')
    tot=$(echo "$tot + $s" | bc -l)
  done
  avg=$(echo "scale=6; $tot / 3" | bc -l)
  [ $t -eq 1 ] && T1=$avg
  printf "%-8s %-12s %-8s\n" $t $avg $(echo "scale=2; $T1 / $avg" | bc -l)
done
echo "logscan total : $(tail -n1 report_8.txt)"
echo "grep -cF total: $(grep -cF "$PAT" "${FILES[@]}" | awk -F: '{s+=$2} END{print s}')"
