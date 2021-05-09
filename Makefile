CC = gcc
CFLAGS = -std=gnu17
CFLAGS += -Wall
# CFLAGS += -Werror
CFLAGS += -Wshadow
CFLAGS += -Wextra
CFLAGS += -fstack-protector-all
CFLAGS += -g

EXEC=fuzzer

all: $(EXEC)

help: help.o
	$(CC) -o $@ $^ $(CFLAGS)

fuzzer: main.o
	$(CC) -o $(EXEC) $^ $(CFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC) help