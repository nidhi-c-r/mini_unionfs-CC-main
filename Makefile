CC = gcc
CFLAGS = -Wall
LIBS = -lfuse3

SRC = src/mini_unionfs.c

all:
	$(CC) $(SRC) $(CFLAGS) $(LIBS) -o mini_unionfs

clean:
	rm -f mini_unionfs