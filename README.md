# GitAdventure

A tiny, playable top-down RPG prototype in C with SDL2. Explore the village, talk to the elder, and defend yourself from a slime.

## Requirements

- C compiler (C11)
- SDL2 development libraries
- CMake 3.16+ (optional; Make is also supported)

### Install SDL2

- Debian/Ubuntu: `sudo apt install build-essential cmake libsdl2-dev`
- macOS (Homebrew): `brew install sdl2`
- Windows: install SDL2 development libraries from [libsdl.org](https://www.libsdl.org/) and configure CMake to find them.

## Build and run

With CMake:

```sh
cmake -S . -B build
cmake --build build
./build/gitadventure
```

With Make and `pkg-config`:

```sh
make
./gitadventure
```

## Controls

- Move: WASD or arrow keys
- Talk to the elder: E (stand nearby)
- Attack: Space (face the slime and get close)
- Quit: Escape

The game uses simple geometric graphics and has no external assets.
