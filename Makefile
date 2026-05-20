CC     = gcc
CFLAGS = -O2 -fopenmp -Wall

TARGETS = analyzer_seq \
          analyzer_par_atomic \
          analyzer_par_critical \
          analyzer_par_lock \
          analyzer_par_atomic_padded

all: $(TARGETS)

analyzer_seq: analyzer_seq.c hash_table.c hash_table.h
	$(CC) $(CFLAGS) analyzer_seq.c hash_table.c -o analyzer_seq

analyzer_par_atomic: analyzer_par_atomic.c hash_table.c hash_table.h common.h
	$(CC) $(CFLAGS) analyzer_par_atomic.c hash_table.c -o analyzer_par_atomic

analyzer_par_critical: analyzer_par_critical.c hash_table.c hash_table.h common.h
	$(CC) $(CFLAGS) analyzer_par_critical.c hash_table.c -o analyzer_par_critical

analyzer_par_lock: analyzer_par_lock.c hash_table.c hash_table.h common.h
	$(CC) $(CFLAGS) analyzer_par_lock.c hash_table.c -o analyzer_par_lock

# mesma fonte do atomic, so muda a flag -DUSE_PADDING
analyzer_par_atomic_padded: analyzer_par_atomic.c hash_table.c hash_table.h common.h
	$(CC) $(CFLAGS) -DUSE_PADDING analyzer_par_atomic.c hash_table.c -o analyzer_par_atomic_padded

clean:
	rm -f $(TARGETS) results.csv sorted_res.csv sorted_gab.csv
