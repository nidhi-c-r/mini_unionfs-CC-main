CC = gcc
CFLAGS = -Wall
LIBS = -lfuse3

SRC = path_utils.c file_ops.c dir_ops.c main.c

all:
	$(CC) $(SRC) $(CFLAGS) $(LIBS) -o mini_unionfs

clean:
	rm -f mini_unionfs