CC = gcc
CFLAGS = -std=gnu17
CFLAGS += -Wall
# CFLAGS += -Werror
CFLAGS += -Wshadow
CFLAGS += -Wextra
CFLAGS += -fstack-protector-all
CFLAGS += -g

EXEC=fuzzer

OBJDIR = obj
SRCDIR = src

all: $(EXEC)

help: $(OBJDIR)/help.o
	$(CC) -o $@ $^ $(CFLAGS)

fuzzer: $(OBJDIR)/main.o $(OBJDIR)/tar.o $(OBJDIR)/fuzz.o
	$(CC) -o $(EXEC) $^ $(CFLAGS)

objdir:
	mkdir $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf $(OBJDIR)/*.o

mrproper: clean
	rm -rf $(EXEC) help

succ:
	rm -rf success_* *.dat