//
// Created by olivier on 18/10/2021.
//

#ifndef OPENLIFE_NATION_H
#define OPENLIFE_NATION_H

typedef struct Homeland
{
	int x, y;
	char *familyName;
} Homeland;

Homeland *getHomeland( int inCenterX, int inCenterY );

#endif //OPENLIFE_NATION_H
