//
// Created by olivier on 18/10/2021.
//

#include "nation.h"

#include "minorGems/util/SimpleVector.h"

SimpleVector<Homeland> homelands;

Homeland *getHomeland( int inCenterX, int inCenterY )
{
	for( int i=0; i<homelands.size(); i++ ) {
		Homeland *h = homelands.getElement( i );

		if( h->x == inCenterX && h->y == inCenterY ) {
			return h;
		}
	}
	return NULL;
}
