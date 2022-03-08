CC=clang
BPF_FLAGS=-I/usr/include/x86_64-linux-gnu -O2 -target bpf
C_FLAGS=-I/usr/include -I/usr/local/include -O2

all: experiment filter.o

filter.o: filter.c
	$(CC) $(BPF_FLAGS) -c $^ -o $@

experiment: experiment.c
	$(CC) $(C_FLAGS) $^ -o $@

clean:
	rm experiment filter.o