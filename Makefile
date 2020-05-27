all:
	clang -o ceditor *.c `sdl2-config --cflags --libs` -lSDL2_ttf
