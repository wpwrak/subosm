/*
 * subosm.c - Obtain useful data about the Buenos Aires Subte from OSM
 *
 * Written 2013 by Werner Almesberger <werner@almesberger.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "db.h"


/* ----- Main -------------------------------------------------------------- */


static void recurse(struct node *n)
{
	struct node **neigh;

	n->tag = 1;
	for (neigh = n->ways; neigh != n->ways+n->n_ways; neigh++) {
		if ((*neigh)->tag)
			continue;
		printf("%d %d\n%d %d\n\n",
		    n->x, n->y, (*neigh)->x, (*neigh)->y);
		recurse(*neigh);
	}
}


static void dump_db(void)
{
	struct node *n;

	for (n = nodes; n != nodes+n_nodes; n++)
		n->tag = 0;
	for (n = nodes; n != nodes+n_nodes; n++) {
		if (n->tag)
			continue;
		if (n != nodes)
			printf("# new net\n\n");
		recurse(n);
	}
}


int main(int argc, char **argv)
{
	read_osm_xml(argv[1]);
	dump_db();
	return 0;
}
