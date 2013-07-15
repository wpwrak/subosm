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

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "local.h"
#include "db.h"


static bool allow_proposed = 0;


/* ----- Distance calculation ---------------------------------------------- */


#define	UNREACHABLE	1000		/* 1 km out */
#define	NEAR		80		/* station "capture" radius, 50 m */


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


static unsigned count_routes(void)
{
	unsigned routes = 0;

	struct node *n;

	for (n = nodes; n != nodes+n_nodes; n++) {
		if (!n->station)
			continue;
		if (n->proposed && !allow_proposed)
			continue;
		routes++;
	}
	return routes;
}


static void find_distances(void)
{
	struct node *n, *m;
	unsigned done = 0, routes;
	int d;

	routes = count_routes();
	for (n = nodes; n != nodes+n_nodes; n++) {
		fprintf(stderr, "%u/%u\r", done, routes);
		fflush(stderr);
		if (!n->station)
			continue;
		if (n->proposed && !allow_proposed)
			continue;
		for (m = nodes; m != nodes+n_nodes; m++) {
			d = hypot(n->x-m->x, n->y-m->y);
			if (d <= NEAR && d < m->distance)
				route(m, d);
		}
		done++;
	}
}


/* ----- Dumping ----------------------------------------------------------- */


static void recurse(struct node *n)
{
	struct edge *edge;

	n->tag = 1;
	for (edge = n->edges; edge != n->edges+n->n_edges; edge++) {
		if (!edge->tag && edge->n->id > n->id)
			printf("%d %d %d # %d\n%d %d %d # %d\n\n",
			    n->x, n->y, n->distance, n->id,
			    edge->n->x, edge->n->y, n->distance, edge->n->id);
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
		if (n->station && (allow_proposed || !n->proposed))
			printf("#STATION %d %d %d # %d\n",
			    n->x, n->y, n->distance, n->id);
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
	if (!strcmp(argv[1], "-p")) {
		allow_proposed = 1;
		argv++;
	}

	lon_min = atof(argv[2]);
	lon_max = atof(argv[3]);
	lat_min = atof(argv[4]);
	lat_max = atof(argv[5]);

	fprintf(stderr, "reading %s\n", argv[1]);
	read_osm_xml(argv[1]);

	fprintf(stderr, "calculating distances\n");
	prepare_routing();
	fprintf(stderr, "routing\n");
	find_distances();
	fprintf(stderr, "writing output\n");
	dump_db();
	return 0;
}
