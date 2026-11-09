# GitAdventure

GitAdventure is a small desktop action RPG prototype built in C with SDL2. It expands from a single village into a larger adventure with multiple areas, quests, monsters, a minimap, and stronger visual polish.

## Features

- Multiple areas: village, forest, and cave
- Quest chain: talk to the elder, gather herbs, defeat monsters, clear the cave guardian
- Enemy variety with different behavior and health
- Minimap to show your position and the map layout
- Custom pixel-style text and simple hand-drawn visual style
- Desktop window app with keyboard controls

## Requirements

- C compiler with C11 support
- SDL2 development libraries
- CMake 3.16+ (or use the Makefile if you prefer)

## macOS setup

```bash
brew install cmake sdl2
```

## Build and run

```bash
git clone https://github.com/jaimehargrave-cpu/GitAdventure.git
cd GitAdventure
cmake -S . -B build
cmake --build build
./build/gitadventure
```

## Controls

- Move: WASD or arrow keys
- Interact: E
- Attack: Space
- Restart after win/lose: R
- Quit: Escape

The game does not use external assets; all visuals are generated in code.
