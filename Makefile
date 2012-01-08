CFLAGS=-Wall -ggdb
LDFLAGS=-fopenmp
ARCH=$(shell uname -m)

PROG=$(ARCH)/radix_sort_serial
PROG+=$(ARCH)/radix_sort_parallel

all: arch $(PROG)

$(ARCH)/%: %.c
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $^

arch:
	mkdir -p $(ARCH)

clean:
	rm -Rf $(PROG) *.o $(ARCH)
