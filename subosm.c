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
#include <math.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "db.h"


/* ----- Distance calculation ---------------------------------------------- */


#define	UNREACHABLE	5000		/* 5 km out */


static void route(struct node *n, int d)
{
	struct edge *e;
	int nd;

	n->distance = d;
	for (e = n->edges; e != n->edges+n->n_edges; e++) {
		nd = d+e->len;
		if (e->n->distance > nd)
			route(e->n, nd);
	}
}


static void prepare_routing(void)
{
	struct node *n;
	struct edge *e;

	for (n = nodes; n != nodes+n_nodes; n++) {
		n->distance = UNREACHABLE;
		for (e = n->edges; e != n->edges+n->n_edges; e++)
			e->len = hypot(n->x-e->n->x, n->y-e->n->y);
	}
}


static void find_distances(void)
{
	struct node *n;

	prepare_routing();
	for (n = nodes; n != nodes+n_nodes; n++)
		if (n->station) {
			route(n, 0);
		}
}


/* ----- Dumping ----------------------------------------------------------- */


static void recurse(struct node *n)
{
	struct edge *edge;

	n->tag = 1;
	for (edge = n->edges; edge != n->edges+n->n_edges; edge++) {
		if (!edge->tag && edge->n->id > n->id)
			printf("%d %d %d # %d%s\n%d %d %d # %d%s\n\n",
			    n->x, n->y, -n->distance, n->id,
			    n->station ? "\tSTATION" : "",
			    edge->n->x, edge->n->y, -n->distance, edge->n->id,
			    edge->n->station ? "\tSTATION" : "");
		edge->tag = 1;
		if (!edge->n->tag)
			recurse(edge->n);
	}
}


static void reset_tags(void)
{
	struct node *n;
	struct edge *e;

	for (n = nodes; n != nodes+n_nodes; n++) {
		n->tag = 0;
		for (e = n->edges; e != n->edges+n->n_edges; e++)
			e->tag = 0;
	}
}


static void dump_db(void)
{
	struct node *n;

	reset_tags();
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


/* ----- Main -------------------------------------------------------------- */


int main(int argc, char **argv)
{
	read_osm_xml(argv[1]);
	find_distances();
	dump_db();
	return 0;
}
