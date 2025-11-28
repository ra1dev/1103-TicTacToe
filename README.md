# Compiling and Running Tic Tac Toe with SDL2 on MSYS2

## 1. Open MSYS2
Launch your MSYS2 terminal.

## 2. Navigate to Your Code Folder
Make sure all the source files are in the current folder.
```bash
cd /path/to/your/code/folder
```

## 3. Update your MSYS2 packages:
```bash
pacman -Syu
```

## 4. Install SDL2 and SDL2_ttf:
```bash
pacman -S mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_ttf
```

## 5. Verify Installation:
Check SDL2 headers:
```bash
ls /ucrt64/include/SDL2
```
You should see files like:
```bash
SDL.h
SDL_ttf.h
```
Check SDL2_ttf libraries:
```bash
ls /ucrt64/lib | grep SDL2_ttf

```
You should see files like:
```bash
libSDL2_ttf.a
libSDL2_ttf.dll.a
```

## 6. Compile Your Code:
Compile using gcc on MSYS2 using this command:
```bash
gcc main.c Minimax.c N_bayes.c playback.c -o ttt.exe   -IC:/msys64/ucrt64/include/SDL2 -LC:/msys64/ucrt64/lib   -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lm -mwindows
```
### Architecture
- All game logic and states handled in one big loop  
- **Fix:** Use a simple state machine or modularize into separate functions/modules (menu, game, results)
