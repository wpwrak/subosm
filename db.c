/*
 * db.c - Topology extracted from OSM
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

#include <expat.h>
#include <glib.h>

#include "local.h"
#include "db.h"


#define	EARTH_R	(6378137/2+6356752/2)	/* meters, (equatorial+polar)/2 */


struct node nodes[MAX_NODES];


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
	void (*end)(void *obj);
	void *obj;
	struct handler *prev;
} *handler;


static struct handler *make_handler(handler_fn fn, void (*end)(void *obj),
    void *obj)
{
	struct handler *h;

	h = alloc_type(struct handler);
	h->fn = fn;
	h->end = end;
	h->obj = obj;
	return h;
}


/* ----- Nodes ------------------------------------------------------------- */


static int node_comp(gconstpointer a, gconstpointer b)
{
	const struct node *na = a, *nb = b;
	
	return na->id - nb->id;
}


/*
 * The projection we use preserves distance along meridians and along circles
 * of latitude.
 */

static void map_coord(struct node *n, double lat, double lon)
{
	double lat_deg_m, lon_deg_m;	/* meters per degree */

	lat_deg_m = EARTH_R/180.0*M_PI;
	lon_deg_m = EARTH_R/180.0*M_PI*cos(lat/180.0*M_PI);

	n->x = (lon-LON_MIN)*lon_deg_m;
	n->y = (lat-LAT_MIN)*lat_deg_m;
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

// dynamic allocation would be cleaner but then we have to recalculate all the
// "way" pointers. Let's do this later :)
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

	/* @@@ Buenos Aires (Capital Federal) */
	if (lon < LON_MIN)
		return NULL;
	if (lon > LON_MAX)
		return NULL;
	if (lat < LAT_MIN)
		return NULL;
	if (lat > LAT_MAX)
		return NULL;

	map_coord(n, lat, lon);

	g_tree_insert(tree, &n->id, n);
	n_nodes++;

	return make_handler(node_handler, NULL, n);
}


/* ----- Ways -------------------------------------------------------------- */


static struct vertex {
	struct node *node;
	struct vertex *prev;	/* we reverse the order */
} *vertices;

static bool keep;


static void link_nodes(struct node *a, struct node *b)
{
	struct edge *e;

	for (e = a->edges; e != a->edges+a->n_edges; e++)
		if (e->n == b) {
			if (verbose)
				fprintf(stderr,
				    "ignoring redundant edge %d -> %d\n",
				    a->id, b->id);
			return;
		}
	a->edges = realloc(a->edges, sizeof(struct edge) * (a->n_edges + 1));
	a->edges[a->n_edges++].n = b;
	n_edges++;
}


static struct handler *way_handler(void *obj, const char *name,
    const char **attr)
{
	struct node *node;
	struct vertex *v;
	int ref = 0;

	if (!strcmp(name, "tag")) {
		if (keep)
			return NULL;
		while (*attr) {
#if 0
			if (!strcmp(attr[0], "v") &&
			    !strcmp(attr[1], "subway")) {
#elif 1
			if (!strcmp(attr[0], "k") &&
			    !strcmp(attr[1], "highway")) {
#endif
				keep = 1;
				break;
			}
			attr += 2;
		}
		return NULL;
	}

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

	v = alloc_type(struct vertex);
	v->node = node;
	v->prev = vertices;
	vertices = v;

	return NULL;
}


static void end_way(void *obj)
{
	struct vertex *v, *prev;

	if (keep)
		for (v = vertices; v && v->prev; v = v->prev) {
			link_nodes(v->node, v->prev->node);
			link_nodes(v->prev->node, v->node);
		}
	while (vertices) {
		prev = vertices->prev;
		free(vertices);
		vertices = prev;
	}
}


static struct handler *way(const char **attr)
{
	vertices = NULL;
	keep = 0;
	return make_handler(way_handler, end_way, NULL);
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
	return make_handler(osm_handler, NULL, NULL);
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
		next = make_handler(null_handler, NULL, NULL);
	next->prev = handler;
	handler = next;
}


static void end(void *user, const char *name)
{
	struct handler *prev;

	if (handler->end)
		handler->end(handler->obj);
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
	handler = make_handler(top_handler, NULL, NULL);

	XML_Parse(parser, map, st.st_size, XML_TRUE);

	fprintf(stderr, "%u nodes %u edges\n", n_nodes, n_edges);
}
