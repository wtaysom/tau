/*
 * Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/times.h>

#define PRd(_x)	printf("%s=%d\n", # _x, _x)

long myRand (void)
{
	static long x = 0;
	
	//x= rand();//x+= 3;
	x= random();//x+= 3;
//	printf(" %d ", x);
	return x;
}

enum { FALSE, TRUE };

void prSet (int *set, int size)
{
	int	i;

	for (i = 0; i < size; ++i) {
		printf(" %d", set[i]);
	}
	printf("\n");
}

int *newSet (int size)
{
	int	*set;
	int	i;
	
	set = malloc(size * sizeof(int));
	if (!set) return set;
	for (i = 0; i < size; ++i) {
		set[i] = -1;
	}
	return set;
}

int findSet (int *set, int x)
{
	if (set[x] < 0) return x;
	else return set[x] = findSet(set, set[x]);
}

void unionSet (int *set, int x, int y)
{
	int	root1 = findSet(set, x);
	int	root2 = findSet(set, y);
	
	if (set[root2] < set[root1]) set[root1] = root2;
	else {
		if (set[root2] == set[root1]) --set[root1];
		set[root2] = root1;
	}
}

typedef struct vInt {
	int		length;
	int		v[1];
} vInt;

typedef struct vvInt {
	int		length;
	vInt	*v[1];
} vvInt;

void prVInt (vInt *v)
{
	int	i;
	
	for (i = 0; i < v->length; ++i) {
		printf("\t%d. %d\n", i, v->v[i]);
	}
}

void prVvInt (vvInt *vv)
{
	int	i;
	
	for (i = 0; i < vv->length; ++i) {
		printf("%d. %p\n", i, vv->v[i]);
		if (vv->v[i]) prVInt(vv->v[i]);
	}
}

void prVirtRow (vInt *v)
{
	int	i;
	
	for (i = 0;;) {
		if (v->v[i]) printf("|");
		else printf(" ");
		++i;
		if (i == v->length) break;
		printf("   ");
	}
	printf("\n");
}

void prHorzRow (vInt *v)
{
	int	i;

	printf("+");
	for (i = 0; i < v->length;++i) {
		if (v->v[i]) printf("---+");
		else printf("   +");
	}
	printf("\n");
}

vInt *newVInt (int length)
{
	vInt	*v;
	
	v = malloc(sizeof(int) * (length + 1));
	if (v) v->length = length;
	return v;
}

void setVInt (vInt *v, int value)
{
	int	i;
	
	for (i = 0; i < v->length; ++i) {
		v->v[i] = value;
	}
}

void prMaze (vvInt *maze)
{
	int	rows = maze->length / 2;
	int	i;
	
	for (i = 0; i < rows; ++i) {
		prHorzRow(maze->v[2*i]);
		prVirtRow(maze->v[1+2*i]);
	}
	prHorzRow(maze->v[2*i]);
}
		
vvInt *makeMaze (int rows, int cols)
{
	vvInt	*vv;
	int		i;

	vv = malloc(sizeof(int) + (2*rows + 1) * sizeof(vInt *));
	vv->length = 2*rows+1;
		/* Horizontal rows */
	for (i = 0; i <= rows; ++i) {
		vv->v[2*i] = newVInt(cols);
		setVInt(vv->v[2*i], 1);
	}
		/* Virtical rows */
	for (i = 0; i < rows; ++i) {
		vv->v[1+2*i] = newVInt(cols+1);
		setVInt(vv->v[1+2*i], 1);
	}
	return vv;
}

int pickAwall (vInt *walls)
{
	int	k;
	int	wall;
	
	if (walls->length == 0) {
		return -1;
	}
	k = myRand() % walls->length;
	wall = walls->v[k];
	walls->v[k] = walls->v[--walls->length];
	return wall;
}

int		Rows = 3;
int		Cols = 4;
int		TotalWalls;
int		VirtWalls;
int		HorzWalls;

int		*Rooms;
vInt	*Walls;

int isVirtWall (int wall, int *row, int *col)
{
	if (wall < VirtWalls) {
		*row = wall / (Cols + 1);
		*col = wall % (Cols + 1);
		return TRUE;
		
	} else {
	
		wall -= VirtWalls;
		
		*row = wall / Cols;
		*col = wall % Cols;
		return FALSE;
	}
}

int adjacentRooms (int wall, int *a, int *b)
{
	int	row;
	int	col;
	
	if (isVirtWall(wall, &row, &col)) {

		if ((col == 0) || (col == Cols)) return -1;
		
		*b = row * Cols + col;
		*a = *b - 1;
	} else {
		
		if ((row == 0) || (row == Rows)) return -1;
		
		*b = row * Cols + col;
		*a = *b - Cols;
	}
	return 0;
}

void discardWall (vvInt *maze, int wall)
{
	int	row;
	int	col;

//printf("%d ", wall);	
	if (isVirtWall(wall, &row, &col)) {
		maze->v[2*row + 1]->v[col] = 0;
	} else {
		maze->v[2*row]->v[col] = 0;
	}
}

void saveWall (vvInt *maze, int wall)
{
	/* Do Nothing */
}

void initWalls (void)
{
	int	i;
	
	VirtWalls = (Cols+1)*Rows;
	HorzWalls = (Rows+1)*Cols;
	TotalWalls = VirtWalls + HorzWalls;

	Walls = newVInt(TotalWalls);
	
	for (i = 0; i < TotalWalls; ++i) {
		Walls->v[i] = i;
	}
}

int main (int argc, char *argv[])
{
	int		rc;
	int		a, b;
	int		wall;
	vvInt	*maze;
	struct tms	time;

//printf("%d\n", times( &time));
	srand(times( &time));
	srandom(times( &time));

#if 0	
{
	int i;
	
	for (i = 0; i < 400; ++i) {
		printf(" %d\n", myRand() % 4);
	}
}
#endif

	if (argc > 1) Rows = atoi(argv[1]);
	if (argc > 2) Cols = atoi(argv[2]);
	
	if ((Rows == 0) || (Cols == 0)) {
		fprintf(stderr, "Usage: %s [rows [columns]]\n", argv[0]);
		return 1;
	}
	initWalls();

	Rooms = newSet(Cols * Rows);
	maze = makeMaze(Rows, Cols);
	
	for (;;) {
		wall = pickAwall(Walls);
		if (wall == -1) break;
		
		rc = adjacentRooms(wall, &a, &b);
		if (rc == -1) {
			saveWall(maze, wall);
			continue;
		}
		if (findSet(Rooms, a) == findSet(Rooms, b)) {
			saveWall(maze, wall);
			continue;
		}
		unionSet(Rooms, a, b);
		discardWall(maze, wall);
	}
//printf("%d\n", Cols);
	discardWall(maze, (myRand() % Cols) + VirtWalls);
	{ int x; //int i;

//	for (i = 0; i < 400; ++i){
	x = (TotalWalls - 1) - (myRand() % Cols);
printf("%d ", x);
//}
//printf("\n");
	discardWall(maze, x);
}
printf("\n");
	prMaze(maze);

	return 0;
}
