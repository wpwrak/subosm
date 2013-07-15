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
	struct edge *edge;

	n->tag = 1;
	for (edge = n->edges; edge != n->edges+n->n_edges; edge++) {
		if (!edge->tag && edge->n->id > n->id)
			printf("%d %d # %d%s\n%d %d # %d%s\n\n",
			    n->x, n->y, n->id,
			    n->station ? "\tSTATION" : "",
			    edge->n->x, edge->n->y, edge->n->id,
			    edge->n->station ? "\tSTATION" : "");
		edge->tag = 1;
		if (!edge->n->tag)
			recurse(edge->n);
	}
}


static void dump_db(void)
{
	struct node *n;
	struct edge *e;

	for (n = nodes; n != nodes+n_nodes; n++) {
		n->tag = 0;
		for (e = n->edges; e != n->edges+n->n_edges; e++)
			e->tag = 0;
	}
	for (n = nodes; n != nodes+n_nodes; n++) {
		if (n->tag)
			continue;
		if (!n->n_edges)
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
