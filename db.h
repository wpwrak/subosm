/*
 * db.h - Topology extracted from OSM
 *
 * Written 2013 by Werner Almesberger <werner@almesberger.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef DB_H
#define	DB_H

#include <stdbool.h>


#define	MAX_NODES	10000000	/* 10 M, big enough for London */


struct edge {
	struct node *n;
	int len;
	int tag;
};

struct node {
	int id;
	int x, y;	/* coordinates (m) */
	bool station;	/* is a subway station */
	bool proposed;	/* station or line is not yet in operation */
	int distance;
	struct edge *edges;
	int n_edges;
	int tag;
};


extern struct node nodes[MAX_NODES];
extern unsigned n_nodes;


void read_osm_xml(const char *name);

#endif /* DB_H */
