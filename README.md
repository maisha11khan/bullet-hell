# Bullet Hell

A small bullet hell shooter written in C++ with [raylib](https://www.raylib.com/).
Dodge waves of bullets, shoot the boss, and survive three attack phases.

## Features
- Object-pooled bullets (up to 10,000 on screen) with smooth performance
- Boss with 3 phases: ring, aimed shots, and a two-arm spiral
- Rest breaks where the boss stops shooting and takes double damage
- Tiny hitbox, slow-move mode, and limited bombs
- Screen shake, particle effects, pause, and start menu
- Resizable window with fullscreen (F11)

## Controls
| Key | Action |
|---|---|
| WASD / Arrow keys | Move |
| Hold SPACE | Shoot |
| Hold SHIFT | Slow move (precise dodging) |
| X | Bomb (clears all bullets, 3 per game) |
| P | Pause |
| F11 | Fullscreen |
| R | Restart (after game over or win) |
| M | Back to menu |

Only the small white dot in the center of your ship can be hit.

## Play
Download `game.exe` from the **Releases** section and run it.
If Windows shows "Windows protected your PC", click **More info**, then **Run anyway**.

## Build from source (Windows)
You need a MinGW-w64 compiler (such as [w64devkit](https://github.com/skeeto/w64devkit)) and the raylib Windows MinGW package.

```
g++ main.cpp -o game.exe -I <raylib>\include -L <raylib>\lib -lraylib -lopengl32 -lgdi32 -lwinmm
```

Replace `<raylib>` with the folder where you extracted raylib.

## Built with
- C++
- raylib 6.0