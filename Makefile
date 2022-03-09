CC=clang
BPF_FLAGS=-I/usr/include/x86_64-linux-gnu -Ibuild/usr/include -O3 -target bpf
C_FLAGS=-I/usr/include -I/usr/local/include -O2

.PHONY: all clean

all: build experiment filter.o

build:
	cd libbpf/src && DESTDIR=../../build make install

filter.o: filter.c
	$(CC) $(BPF_FLAGS) -c $^ -o $@

experiment: experiment.c
	$(CC) $(C_FLAGS) $^ -o $@

clean:
	rm experiment filter.o