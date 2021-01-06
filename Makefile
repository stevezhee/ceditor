CFLAGS= $(shell sdl2-config --cflags) -g
LIBS=$(shell sdl2-config --libs) -lSDL2_ttf
CFILES=$(shell ls *.c)
OFILES=$(CFILES:.c=.o)
HFILES=$(shell ls *.h)

all: ceditor

run: ceditor
	./ceditor *.txt *.[ch]

ceditor: $(OFILES)
	clang $(LIBS) -o $@ $^

%.o: %.c $(HFILES)
	clang -c $(CFLAGS) $<

pretty:
	clang-format -i *.[ch]

checkpoint:
	-git commit -am"cp"

debug: ceditor
	lldb -- ./ceditor main.c Util.c Util.h

clean:
	rm -rf ceditor $(OFILES)
