CFLAGS=$(shell sdl2-config --cflags)
LIBS=$(shell sdl2-config --libs) -lSDL2_ttf
CFILES=$(shell ls *.c)
OFILES=$(CFILES:.c=.o)

all: ceditor
	./ceditor *.[ch]

ceditor: $(OFILES)
	clang $(LIBS) -o $@ $^

%.o: %.c
	clang -c $(CFLAGS) $^

clean:
	rm -rf ceditor $(OFILES)
