#include "GridPos.h"

#include <math.h>

double distance( GridPos inA, GridPos inB ) {
    double dx = (double)inA.x - (double)inB.x;
    double dy = (double)inA.y - (double)inB.y;

    return sqrt(  dx * dx + dy * dy );
    }

double distance2( GridPos inA, GridPos inB )
{
	int dX = inA.x - inB.x;
	int dY = inA.y - inB.y;

	return dX * dX + dY * dY;
}

char equal( GridPos inA, GridPos inB ) {
	return inA.x == inB.x && inA.y == inB.y;
}
