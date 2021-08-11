//
// Created by olivier on 09/08/2021.
//

#include "linearDB.h"

#include <iostream>

#include "OneLife/server/lineardb3.h"
#include "OneLife/server/dbCommon.h"

extern char anyBiomesInDB;
extern int maxBiomeXLoc;
extern int maxBiomeYLoc;
extern int minBiomeXLoc;
extern int minBiomeYLoc;

extern LINEARDB3 biomeDB;

double gapIntScale = 1000000.0;

common::object::store::memory::randomAccess::LinearDB::LinearDB(LINEARDB3* dbState)
{
	this->dbState = dbState;
}

common::object::store::memory::randomAccess::LinearDB::~LinearDB() {}

int common::object::store::memory::randomAccess::LinearDB::get(int idx)
{
	return 0;
}

void common::object::store::memory::randomAccess::LinearDB::put(int idx, int value) {}





// returns -1 if not found
int biomeDBGet( int inX, int inY,
			  int *outSecondPlaceBiome,
			  double *outSecondPlaceGap)
			  {
	unsigned char key[8];
	unsigned char value[12];

	// look for changes to default in database
	intPairToKey( inX, inY, key );

	int result = LINEARDB3_get( &biomeDB, key, value );

	if( result == 0 ) {
		// found
		int biome = valueToInt( &( value[0] ) );

		if( outSecondPlaceBiome != NULL ) {
			*outSecondPlaceBiome = valueToInt( &( value[4] ) );
		}

		if( outSecondPlaceGap != NULL ) {
			*outSecondPlaceGap = valueToInt( &( value[8] ) ) / gapIntScale;
		}

		return biome;
	}
	else {
		return -1;
	}
}