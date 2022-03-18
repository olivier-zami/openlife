#ifndef GRID_POS_INCLUDED
#define GRID_POS_INCLUDED

typedef struct GridPos {
        int x;
        int y;
    } GridPos;


double distance( GridPos inA, GridPos inB );
char equal( GridPos inA, GridPos inB );
double distance2( GridPos inA, GridPos inB );

#endif
