# GitAdventure

A small, playable top-down RPG for desktop, written in C with SDL2. Help the village elder, collect three glowing herbs, defeat the slime, and return to the elder to complete the quest. The game draws its pixel-style graphics and text itself, so no extra assets are needed.

## Requirements

- C compiler (C11)
- SDL2 development libraries
- CMake 3.16+ (or Make and `pkg-config`)

### Install dependencies

- macOS with Homebrew: `brew install cmake sdl2`
- Debian/Ubuntu: `sudo apt install build-essential cmake libsdl2-dev`
- Windows: install SDL2 development libraries from [libsdl.org](https://www.libsdl.org/) and configure CMake to find them.

## Build and run

First clone the repository, then run these commands from inside the `GitAdventure` folder:

```sh
git clone https://github.com/jaimehargrave-cpu/GitAdventure.git
cd GitAdventure
cmake -S . -B build
cmake --build build
./build/gitadventure
```

If you already cloned it, just open Terminal in that folder and run the last three commands. Check your current folder with `pwd`; it should end in `/GitAdventure`. `CMakeLists.txt` is in the repository root.

On macOS, if CMake cannot find SDL2, run `brew install sdl2` and retry. With Make and `pkg-config`, you can also run `make` then `./gitadventure`.

## How to play

1. Walk up to the elder and press **E** to accept the quest.
2. Explore the village and press **E** beside each glowing herb to collect it. The HUD tracks your progress.
3. Find the slime and press **Space** while facing it and close by to attack. Avoid touching it or you will lose health.
4. Return to the elder and press **E** to finish the quest.

- Move: **WASD** or arrow keys
- Interact / collect: **E**
- Attack: **Space**
- Restart after winning or losing: **R**
- Quit: **Esc**
