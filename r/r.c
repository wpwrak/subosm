#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>
#include <SDL_gfxPrimitives.h>

#include "popen2.h"


#define	GOOD	0x00ff00ff
#define	AVERAGE	0xffe020ff
#define	BAD	0xff2020ff
#define	REMOTE	0xffffffff
#define	ROAD	0xb0b0b0ff
#define	STATION	0x1010ffff

#define	STATION_R 1


enum class {
	good,
	average,
	bad,
	remote
};


/* ----- Database ---------------------------------------------------------- */


#define	MAX_STATIONS	10000	/* 10 k */
#define	MAX_NODES	1000000	/* 10 M */
#define	MAX_EDGES	1000000	/* 10 M */
#define	MAX_FACES	1000000	/* 10 M */


static struct station {
	int x, y;
} station[MAX_STATIONS];

static struct node {
	int x, y, d;
} node[MAX_NODES];

static struct edge {
	unsigned a, b;	/* node index */
} edge[MAX_EDGES];

static struct face {
	unsigned a, b, c;	/* node index */
} face[MAX_FACES];

static unsigned n_stations, n_nodes, n_edges, n_faces;



static void add_station(int x, int y)
{
	station[n_stations].x = x;
	station[n_stations].y = y;
	n_stations++;
}


static unsigned add_node(int x, int y, int d)
{
	node[n_nodes].x = x;
	node[n_nodes].y = y;
	node[n_nodes].d = d;
	return n_nodes++;
}


static void add_edge(unsigned a, unsigned b)
{
	edge[n_edges].a = a;
	edge[n_edges].b = b;
	n_edges++;
}


static void add_face(unsigned a, unsigned b, unsigned c)
{
	face[n_faces].a = a;
	face[n_faces].b = b;
	face[n_faces].c = c;
	n_faces++;
}


static void read_gp(FILE *file)
{
	char buf[100];
	int x, y, d;
	int n;
	int this, last = -1;

	while (fgets(buf, sizeof(buf), stdin)) {
		n = sscanf(buf, "#STATION %d %d", &x, &y);
		if (n == 2) {
			add_station(x, y);
			continue;
		}
		if (!strcmp(buf, "\n")) {
			last = -1;
			continue;
		}
		n = sscanf(buf, "%d %d %d", &x, &y, &d);
		if (n != 3)
			continue;
		this = add_node(x, y, d);
		if (last == -1) {
			last = this;
		} else {
			add_edge(last, this);
			last = -1;
		}
	}
}


/* ----- Triangulation ----------------------------------------------------- */


static void triangulate(void)
{
	FILE *files[2];
	unsigned i, faces;
	unsigned a, b, c;
	int n;

	/* start qdelaunay */

	if (popen2("qdelaunay Qt i", files) < 0) {
		perror("popen2");
		exit(1);
	}

	/* send list of points */

	if (fprintf(files[1], "2\n%u\n", n_nodes) < 0) {
		perror("fprintf");
		exit(1);
	}
	for (i = 0; i != n_nodes; i++) {
		if (fprintf(files[1], "%d %d\n", node[i].x, node[i].y) < 0) {
			perror("fprintf");
			exit(1);
		}
	}
	if (fclose(files[1]) < 0) {
		perror("fclose");
		exit(1);
	}

	/* read list of faces (triangles) */

	n = fscanf(files[0], "%u", &faces);
	if (n < 0) {
		perror("fscanf");
		exit(1);
	}
	if (n != 1) {
		fprintf(stderr, "bad result (length)\n");
		exit(1);
	}
	for (i = 0; i != faces; i++) {
		n = fscanf(files[0], "%u %u %u", &a, &b, &c);
		if (n < 0) {
			perror("fscanf");
			exit(1);
		}
		if (n != 3) {
			fprintf(stderr, "bad result (face)\n");
			exit(1);
		}
		add_face(a, b, c);
	}
	fclose(files[0]);
}


/* ----- Plotting ---------------------------------------------------------- */


static unsigned int xmin, ymax;
static double f;


static SDL_Surface *make_canvas(int height)
{
	SDL_Surface *s;
	int xmax, ymin;
	unsigned i;
	unsigned dx, dy;
	unsigned width;

	xmin = xmax = node[0].x;
	ymin = ymax = node[0].y;

	for (i = 1; i != n_nodes; i++)  {
		if (xmin > node[i].x)
			xmin = node[i].x;
		if (xmax < node[i].x)
			xmax = node[i].x;
		if (ymin > node[i].y)
			ymin = node[i].y;
		if (ymax < node[i].y)
			ymax = node[i].y;
	}

	dx = xmax-xmin;
	dy = ymax-ymin;
	width = height*dx/dy;

	f = (double) height/dy;

	s = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32,
	    0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	boxColor(s, 0, 0, width-1, height-1, 0xffffffff);
	return s;
}


static inline int X(int x)
{
	return (x-xmin)*f;
}


static inline int Y(int y)
{
	return (ymax-y)*f;
}


static enum class classify(int d)
{
	if (d <= 333)
		return good;
	if (d <= 666)
		return average;
	if (d < 1000)
		return bad;
	return remote;
}


static double split_at(int at, int a, int b)
{
	return (double) (at-a)/(b-a);
}


static double split(int a, int b)
{
	enum class ca, cb;

	ca = classify(a);
	cb = classify(b);

	if (ca == cb)
		return 0.5;
	if (ca > cb)
		return 1.0-split(b, a);

	switch (ca) {
	case good:
		switch (cb) {
		case average:
			return split_at(333, a, b);
		case bad:
			return split_at(500, a, b);
		case remote:
			return split_at(999, a, b);
		default:
			abort();
		}
	case average:
		switch (cb) {
		case bad:
			return split_at(666, a, b);
		case remote:
			return split_at(999, a, b);
		default:
			abort();
		}
	case bad:
		switch (cb) {
		case remote:
			return split_at(1000, a, b);
		default:
			abort();
		}
	default:
		abort();
	}
}


static void corner(SDL_Surface *s, int ax, int ay, int bx, int by,
    int cx, int cy, int mx, int my, uint32_t color)
{
	filledTrigonColor(s, X(ax), Y(ay), X(bx), Y(by), X(cx), Y(cy), color);
	filledTrigonColor(s, X(mx), Y(my), X(bx), Y(by), X(cx), Y(cy), color);
}


static void paint_triangle(SDL_Surface *s, const struct node *a,
    const struct node *b, const struct node *c)
{
	static const uint32_t color[] = {
		[good]		= GOOD,
		[average]	= AVERAGE,
		[bad]		= BAD,
		[remote]	= REMOTE
	};
	uint32_t ca, cb, cc;
	double wab, wac, wbc;
	int mabx, maby, macx, macy, mbcx, mbcy;
	int mx, my;

	/* colors of corners */

	ca = color[classify(a->d)];
	cb = color[classify(b->d)];
	cc = color[classify(c->d)];

	/* find where on a side the border is */

	wab = split(a->d, b->d);
	wac = split(a->d, c->d);
	wbc = split(b->d, c->d);

	/* border points on sides */

	mabx = wab*a->x+(1-wab)*b->x;
	maby = wab*a->y+(1-wab)*b->y;
	macx = wac*a->x+(1-wac)*c->x;
	macy = wac*a->y+(1-wac)*c->y;
	mbcx = wbc*b->x+(1-wbc)*c->x;
	mbcy = wbc*b->y+(1-wbc)*c->y;

	/* centroid of the triangle formed by the border points */

	mx = (mabx+macx+mbcx)/3;
	my = (maby+macy+mbcy)/3;

	corner(s, a->x, a->y, mabx, maby, macx, macy, mx, my, ca);
	corner(s, b->x, b->y, mabx, maby, mbcx, mbcy, mx, my, cb);
	corner(s, c->x, c->y, macx, macy, mbcx, mbcy, mx, my, cc);
}


static void paint_triangles(SDL_Surface *s)
{
	unsigned i;
	const struct node *a, *b, *c;

	for (i = 0; i != n_faces; i++) {
		a = node+face[i].a;
		b = node+face[i].b;
		c = node+face[i].c;
		paint_triangle(s, a, b, c);
	}
}


static void draw_net(SDL_Surface *s)
{
	unsigned i;
	const struct node *a, *b;

	for (i = 0; i != n_edges; i++) {
		a = node+edge[i].a;
		b = node+edge[i].b;
		lineColor(s, X(a->x), Y(a->y), X(b->x), Y(b->y), ROAD);
	}
}


static void draw_stations(SDL_Surface *s)
{
	unsigned i;

	for (i = 0; i != n_stations; i++)
		filledCircleColor(s, X(station[i].x), Y(station[i].y),
		    STATION_R, STATION);
}


static void do_gfx(void)
{
	SDL_Surface *s;

	s = make_canvas(2*600);
	paint_triangles(s);
	draw_net(s);
	draw_stations(s);
	SDL_SaveBMP(s, "bmp");
}


static void dump_tri(void)
{
	unsigned i;
	const struct node *a, *b, *c;

	for (i = 0; i != n_faces; i++) {
		a = node+face[i].a;
		b = node+face[i].b;
		c = node+face[i].c;
		printf("%d %d\n%d %d\n%d %d\n%d %d\n\n",
		    a->x, a->y, b->x, b->y, c->x, c->y, a->x, a->y);
	}
}

/* ----- Main -------------------------------------------------------------- */


int main(int argc, char **argv)
{
	read_gp(stdin);
	triangulate();
//dump_tri();
	do_gfx();
	return 0;
}
