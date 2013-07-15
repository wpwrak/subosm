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
plot "$1" using 1:2:3 with lines palette, \
  "<grep STATION out" with points lt 1 pt 7
EOF
