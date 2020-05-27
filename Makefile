all:
	clang -o editor *.c `sdl2-config --cflags --libs` -lSDL2_ttf
