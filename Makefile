CC=gcc
CFLAGS=-Wall -Wextra -O2
all: pipeline copy_sys copy_stdio perfect logscan
perfect: perfect.c
	$(CC) $(CFLAGS) -pthread -o $@ $<
logscan: logscan.c
	$(CC) $(CFLAGS) -pthread -o $@ $<
%: %.c
	$(CC) $(CFLAGS) -o $@ $<
clean:
	rm -f pipeline copy_sys copy_stdio perfect logscan result.txt out.bin
