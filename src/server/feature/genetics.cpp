//
// Created by olivier on 18/10/2021.
//

#include "genetics.h"

// false for male, true for female
char getFemale( LiveObject *inPlayer )
{
	ObjectRecord *r = getObject( inPlayer->displayID );

	return ! r->male;
}