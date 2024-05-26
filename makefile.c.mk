CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Wl,--stack,1000000000
LIBS = -lm

all: image run

image: main.c lodepng.c lodepng.h
	$(CC) $(CFLAGS) -o image main.c lodepng.c $(LIBS)

run: image
	./image

clean:
	rm -f image
