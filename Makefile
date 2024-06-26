CC=gcc
CFLAGS=-std=c17 -Wall -Wextra -Werror
LIBS=-L.\SDL2-2.30.3\x86_64-w64-mingw32\lib -L.\SDL2_image-2.8.2\x86_64-w64-mingw32\lib -lmingw32 -lSDL2main -lSDL2_image -lSDL2
INCLUDES=-I.\SDL2-2.30.3\x86_64-w64-mingw32\include\SDL2 -I.\SDL2_image-2.8.2\x86_64-w64-mingw32\include\SDL2

all:
	$(CC) app.c -o app $(CFLAGS) $(LIBS) $(INCLUDES)
