#
# Makefile - Build subway coverage mapper
#
# Written 2013 by Werner Almesberger <werner@almesberger.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#

NAME = subosm

MAP = buenos-aires.osm
MAP_DISTFILE = buenos-aires.osm.bz2
MAP_DL = http://osm-extracted-metros.s3.amazonaws.com/$(MAP_DISTFILE)

CFLAGS = -Wall -g `pkg-config --cflags glib-2.0`
LDLIBS = -lexpat `pkg-config --libs glib-2.0` -lm

OBJS = $(NAME).o db.o

.PHONY:		all run plot clean spotless

all:		$(NAME)

$(NAME):	$(OBJS)
		$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

clean:
		rm -f $(OBJS)

run:		subosm $(MAP)
		./subosm buenos-aires.osm >out

plot:
		./plot out

$(MAP_DISTFILE):
		wget $(MAP_DL) || { rm -f $@; exit 1; }

%:		%.bz2
		bunzip2 -k $<

spotless:	clean
		rm -f $(NAME)
