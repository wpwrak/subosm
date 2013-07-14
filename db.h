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


#define	MAX_NODES	1000000


struct edge {
	struct node *n;
	int tag;
};

struct node {
	int id;
	int x, y;	/* coordinates (m) */
	struct edge *edges;
	int n_edges;
	int tag;
} nodes[MAX_NODES];

unsigned n_nodes;


void read_osm_xml(const char *name);

#endif /* DB_H */
