CFLAGS = -Wall -g `sdl-config --cflags` `pkg-config --cflags SDL_gfx`
LDLIBS = `sdl-config --libs` `pkg-config --libs SDL_gfx`

OBJS = r.o popen2.o

.PHONY:	run show clean

r:	$(OBJS)

run:	r
	./r <../BUE.gp

show:
	display bmp

clean:
	rm -f $(OBJS)
