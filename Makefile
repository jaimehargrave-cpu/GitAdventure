CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -std=c11
SDL_CFLAGS := $(shell pkg-config --cflags sdl2)
SDL_LIBS := $(shell pkg-config --libs sdl2)

.PHONY: all clean
all: gitadventure

gitadventure: src/main.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ $< $(SDL_LIBS)

clean:
	rm -f gitadventure
