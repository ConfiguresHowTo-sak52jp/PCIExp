CC := aarch64-linux-gnu-gcc
CFLAGS := -I. -I../driver -std=c99 -Wall -O0 -static

TARGETS := app1

all: $(TARGETS)

app1: app1.c
	$(CC) $(CFLAGS) -o $@ $<


clean:
	rm -f $(TARGETS) *.o *~
