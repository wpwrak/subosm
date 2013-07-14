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


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <expat.h>
#include <glib.h>

#include "db.h"


#define	SCALE	10000000


struct node nodes[1000000];


static GTree *tree;
static unsigned n_edges;
static int verbose = 0;


/* ----- Helper functions -------------------------------------------------- */


#define	alloc_type(t) (t *) malloc(sizeof(t))


/* ----- Handlers ---------------------------------------------------------- */


typedef struct handler *(*handler_fn)(void *obj, const char *name,
    const char **attr);

static struct handler {
	handler_fn fn;
	void *obj;
	struct handler *prev;
} *handler;


static struct handler *make_handler(handler_fn fn, void *obj)
{
	struct handler *h;

	h = alloc_type(struct handler);
	h->fn = fn;
	h->obj = obj;
	return h;
}


/* ----- Nodes ------------------------------------------------------------- */


static int node_comp(gconstpointer a, gconstpointer b)
{
	const struct node *na = a, *nb = b;
	
	return na->id - nb->id;
}


static void map_coord(struct node *n, double lat, double lon)
{
	n->x = lon*SCALE;
	n->y = lat*SCALE;
}


static struct handler *node_handler(void *obj, const char *name,
    const char **attr)
{
	return NULL;
}


static struct handler *node(const char **attr)
{
	double lat, lon;
	struct node *n;

//	nodes = realloc(nodes, sizeof(struct node) * (n_nodes+1));
	n = nodes + n_nodes;

	memset(n, 0, sizeof(*n));

	while (*attr) {
		if (!strcmp(attr[0], "id"))
			n->id = atoi(attr[1]);
		else if (!strcmp(attr[0], "lat"))
			lat = atof(attr[1]);
		else if (!strcmp(attr[0], "lon"))
			lon = atof(attr[1]);
		attr += 2;
	}

	if (lon < -58.54)
		return NULL;
	if (lon > -58.33)
		return NULL;
	if (lat < -34.71)
		return NULL;
	if (lat > -34.53)
		return NULL;

	map_coord(n, lat, lon);

	g_tree_insert(tree, &n->id, n);
	n_nodes++;

	return make_handler(node_handler, n);
}


/* ----- Ways -------------------------------------------------------------- */


static struct node *last_node;


static void link_nodes(struct node *a, struct node *b)
{
	struct node **n;

	for (n = a->ways; n != a->ways+a->n_ways; n++)
		if (*n == b) {
			if (verbose)
				fprintf(stderr,
				    "ignoring redundant edge %d -> %d\n",
				    a->id, b->id);
			return;
		}
	a->ways = realloc(a->ways, sizeof(struct node *) * (a->n_ways + 1));
	a->ways[a->n_ways++] = b;
	n_edges++;
}


static struct handler *way_handler(void *obj, const char *name,
    const char **attr)
{
	struct node *node;
	int ref = 0;

	if (strcmp(name, "nd"))
		return NULL;
	while (*attr) {
		if (!strcmp(attr[0], "ref")) {
			ref = atoi(attr[1]);
			break;
		}
		attr += 2;
	}

	node = g_tree_lookup(tree, &ref);
	if (!node) {
		if (verbose)
			fprintf(stderr, "unknown node %d\n", ref);
		return NULL;
	}

	if (last_node) {
		link_nodes(last_node, node);
		link_nodes(node, last_node);
	}

	last_node = node;

	return NULL;
}


static struct handler *way(const char **attr)
{
	last_node = NULL;
	return make_handler(way_handler, NULL);
}


/* ----- OSM handler ------------------------------------------------------- */


static struct handler *osm_handler(void *obj, const char *name,
    const char **attr)

{
	if (!strcmp(name, "node"))
		return node(attr);
	if (!strcmp(name, "way"))
		return way(attr);
	return NULL;
}


static struct handler *osm(const char **attr)
{
	return make_handler(osm_handler, NULL);
}


/* ----- Top-level handler ------------------------------------------------- */


static struct handler *null_handler(void *obj, const char *name,
    const char **attr)
{
	return NULL;
}


static struct handler *top_handler(void *obj, const char *name,
    const char **attr)
{
	if (!strcmp(name, "osm"))
		return osm(attr);
	return NULL;
}


static void start(void *user, const char *name, const char **attr)
{
	struct handler *next;

	next = handler->fn(handler, name, attr);
	if (!next)
		next = make_handler(null_handler, NULL);
	next->prev = handler;
	handler = next;
}


static void end(void *user, const char *name)
{
	struct handler *prev;

	prev = handler->prev;
	free(handler);
	handler = prev;
}



/* ----- XML parser -------------------------------------------------------- */


void read_osm_xml(const char *name)
{
	int fd;
	struct stat st;
	XML_Parser parser;
	void *map;

	parser = XML_ParserCreate(NULL);
	XML_SetElementHandler(parser, start, end);

	fd = open(name, O_RDONLY);
	if (fd < 0) {
		perror(name);
		exit(1);
	}

	if (fstat(fd, &st) < 0) {
		perror("fstat");
		exit(1);
	}

	map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map == (void *) -1) {
		perror("mmap");
		exit(1);
	}

	tree = g_tree_new(node_comp);
	handler = make_handler(top_handler, NULL);

	XML_Parse(parser, map, st.st_size, XML_TRUE);

	fprintf(stderr, "%u nodes %u edges\n", n_nodes, n_edges);
}
