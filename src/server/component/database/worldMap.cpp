//
// Created by olivier on 07/08/2021.
//

#include "worldMap.h"
#include "OneLife/server/map.h"


int getMapBiomeIndex( int inX, int inY,
							 int *outSecondPlaceIndex,
							 double *outSecondPlaceGap) {

	int secondPlaceBiome = -1;

	int dbBiome = -1;

	if( anyBiomesInDB &&
	inX >= minBiomeXLoc && inX <= maxBiomeXLoc &&
	inY >= minBiomeYLoc && inY <= maxBiomeYLoc ) {
		// don't bother with this call unless biome DB has
		// something in it, and this inX,inY is in the region where biomes
		// exist in the database (tutorial loading, or test maps)
		dbBiome = biomeDBGet( inX, inY,
							  &secondPlaceBiome,
							  outSecondPlaceGap );
	}


	if( dbBiome != -1 ) {

		int index = getBiomeIndex( dbBiome );

		if( index != -1 ) {
			// biome still exists!

			char secondPlaceFailed = false;

			if( outSecondPlaceIndex != NULL ) {
				int secondIndex = getBiomeIndex( secondPlaceBiome );

				if( secondIndex != -1 ) {

					*outSecondPlaceIndex = secondIndex;
				}
				else {
					secondPlaceFailed = true;
				}
			}

			if( ! secondPlaceFailed ) {
				return index;
			}
		}
		else {
			dbBiome = -1;
		}

		// else a biome or second place in biome.db that isn't in game anymore
		// ignore it
	}


	int secondPlace = -1;

	double secondPlaceGap = 0;


	int pickedBiome = computeMapBiomeIndex( inX, inY,
											&secondPlace, &secondPlaceGap );


	if( outSecondPlaceIndex != NULL ) {
		*outSecondPlaceIndex = secondPlace;
	}
	if( outSecondPlaceGap != NULL ) {
		*outSecondPlaceGap = secondPlaceGap;
	}


	if( dbBiome == -1 || secondPlaceBiome == -1 ) {
		// not stored, OR some part of stored stale, re-store it

		secondPlaceBiome = 0;
		if( secondPlace != -1 ) {
			secondPlaceBiome = biomes[ secondPlace ];
		}

		// skip saving proc-genned biomes for now
		// huge RAM impact as players explore distant areas of map

		// we still check the biomeDB above for loading test maps
		/*
        biomeDBPut( inX, inY, biomes[pickedBiome],
                    secondPlaceBiome, secondPlaceGap );
        */
	}

	return pickedBiome;
}

