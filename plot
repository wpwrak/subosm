#!/bin/sh
#
# plot - Plot the generated map data
#
# Written 2013 by Werner Almesberger <werner@almesberger.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#

gnuplot --persist <<EOF
set title "Distance (km) from subway station"
set size ratio -1
set palette model RGB
set palette model RGB defined \
  (0 "green", 3 "green", 3 "yellow", 6 "yellow", 6 "red", 9.9 "red", \
   9.9 "grey", 10 "grey")
plot "$1" using 1:2:(\$3/1000) with lines palette notitle, \
  "<sed '/#STATION /s///p;d' out" with points lt 3 pt 7 ps 0.3 notitle
EOF
