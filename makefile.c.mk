CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LIBS = -lm

all: main run

image: main.c lodepng.c lodepng.h
 $(CC) $(CFLAGS) -o image main.c lodepng.c $(LIBS)



run: main
 ./main

clean:
 rm -f coloring main
