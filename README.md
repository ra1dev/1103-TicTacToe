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
