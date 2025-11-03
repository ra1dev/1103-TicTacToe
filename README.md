1. open msys

2. navigate to your code folder (with tic_tac_toe_sdl.c inside)

3. pacman -Syu

4. pacman -S mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_ttf

5. to verify installation, run ls /ucrt64/include/SDL2 and you should see: 
SDL.h
SDL_ttf.h

6. run ls /ucrt64/lib | grep SDL2_ttf and you should see:
libSDL2_ttf.a
libSDL2_ttf.dll.a

7. run:
gcc tic_tac_toe_sdl.c -o tic_tac_toe_sdl.exe \
  -IC:/msys64/ucrt64/include/SDL2 \
  -LC:/msys64/ucrt64/lib \
  -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lm -mwindows

8. run ./tic_tac_toe_sdl.exe

TO COMPILE:
gcc tic_tac_toe_sdl.c -o tic_tac_toe_sdl.exe -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lm

# Compiling and Running Tic Tac Toe with SDL2 on MSYS2

## 1. Open MSYS2
Launch your MSYS2 terminal.

## 2. Navigate to Your Code Folder
Make sure your `tic_tac_toe_sdl.c` file is in the current folder.
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
Compile using gcc with SDL2 and SDL2_ttf:
```bash
gcc tic_tac_toe_sdl.c -o tic_tac_toe_sdl.exe \ -IC:/msys64/ucrt64/include/SDL2 \ -LC:/msys64/ucrt64/lib \ -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lm -mwindows

gcc tic_tac_toe_sdl.c -o tic_tac_toe_sdl.exe -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lm
```

## Inefficiencies and Suggested Fixes

### Rendering
- Recreates text textures every frame  
- **Fix:** Cache or reuse textures instead of recreating them each loop  

---

### Logic
- Uses full minimax algorithm without pruning (explores all possible game states)  
- **Fix:** Implement alpha–beta pruning or memoization to reduce computation  

---

### Events
- Uses polling loop with `SDL_Delay()` (wastes CPU cycles)  
- **Fix:** Replace with `SDL_WaitEvent()` or `SDL_WaitEventTimeout()` for event-driven updates  

---

### Memory
- Leaks textures in loops (e.g., menu textures recreated every iteration)  
- **Fix:** Properly destroy textures or move their creation outside loops  

---

### Responsiveness
- Uses blocking calls like `SDL_Delay(2000)` which freeze rendering  
- **Fix:** Replace with non-blocking timers or per-frame countdown logic  

---

### Robustness
- Missing error checks and array bounds validation  
- **Fix:** Validate all SDL and TTF return values; check mouse click bounds  

---

### Architecture
- All game logic and states handled in one big loop  
- **Fix:** Use a simple state machine or modularize into separate functions/modules (menu, game, results)
