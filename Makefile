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

CITY ?= BUE

CITIES = BUE LON MAD NYC PAR VIE

# OSM data file
BUE = buenos-aires
LON = london
MAD = madrid
NYC = new-york
PAR = paris
VIE = vienna

# city		longitude	latitude
BUE_RECT =	-58.54 -58.33	-34.71 -34.53
LON_RECT =	 -0.51   0.33	 51.29  51.70
MAD_RECT =	 -3.84  -3.52	 40.30  40.56
NYC_RECT =	-74.04 -73.70	 40.54  40.92
PAR_RECT =	  2.22   2.46	 48.80  48.92
VIE_RECT =	 16.18  16.58	 48.11  48.32

# city		xrange		yrange		pixels
BUE_PLOT = 	 0.5	18	 0	19.5	700	600
LON_PLOT = 	18.5	34	21	32	800 	500
MAD_PLOT =	 4.6	21.9	 6.1	22	650	650
NYC_PLOT =	 0	10.7	12.7	30.3	550	800
PAR_PLOT =	 0	17.5	 0	13.3	800	600
VIE_PLOT =	 6	22	 4.5	18.5	800	600

THUMB_SIZE = 120 120

OUTDIR ?= .

MAP = $($(CITY)).osm
MAP_DISTFILE = $(MAP).bz2
MAP_DL = http://osm-extracted-metros.s3.amazonaws.com/$(MAP_DISTFILE)

CFLAGS = -Wall -g `pkg-config --cflags glib-2.0`
LDLIBS = -lexpat `pkg-config --libs glib-2.0` -lm

OBJS = $(NAME).o db.o

.PHONY:		all run plot clean spotless
.PHONY:		thumb png forall web

all:		$(NAME)

$(NAME):	$(OBJS)
		$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

clean:
		rm -f $(OBJS)

forall:
		for n in $(CITIES); do $(MAKE) CITY=$$n $(CMD); done

web:
		$(MAKE) OUTDIR=web forall CMD=thumb
		$(MAKE) OUTDIR=web forall CMD=png

run:		subosm $(MAP)
		./subosm $(MAP) $($(CITY)_RECT) >$(CITY).gp

plot:
		./plot $(CITY).gp

png:
		./plot $(CITY).gp $(OUTDIR)/$(CITY).png $($(CITY)_PLOT)

thumb:
		./plot -t $(CITY).gp $(OUTDIR)/$(CITY)-thumb.png $(THUMB_SIZE)

$(MAP_DISTFILE):
		wget $(MAP_DL) || { rm -f $@; exit 1; }

%:		%.bz2
		bunzip2 -k $<

spotless:	clean
		rm -f $(NAME)
