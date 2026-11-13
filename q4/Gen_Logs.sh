#!/bin/bash
# Generate 12 log files of different sizes (1-50 MB) in ./logs
mkdir -p logs
sizes=(1 2 3 5 8 10 15 20 25 30 40 50)
i=1
for mb in "${sizes[@]}"; do
  awk -v lines=$((mb*13000)) -v seed=$i 'BEGIN{
    srand(seed); split("INFO DEBUG WARN ERROR",L," ");
    for(n=0;n<lines;n++){
      r=rand(); lv=(r<0.55)?1:(r<0.85)?2:(r<0.95)?3:4;
      printf "2026-11-%02d %02d:%02d:%02d [%s] service-%d request id=%d status=%d latency=%dms\n",
        1+int(rand()*28),int(rand()*24),int(rand()*60),int(rand()*60),L[lv],
        int(rand()*20),int(rand()*1000000),200+int(rand()*4)*100,int(rand()*900)
    }}' > logs/app$(printf %02d $i).log
  i=$((i+1))
done
ls -lh logs
