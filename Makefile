CC = gcc
CFLAGS = -Wall
LIBS = -lfuse3

SRC = src/path_utils.c src/file_ops.c src/dir_ops.c src/main.c

all:
	$(CC) $(SRC) $(CFLAGS) $(LIBS) -o mini_unionfs

clean:
	rm -f mini_unionfs