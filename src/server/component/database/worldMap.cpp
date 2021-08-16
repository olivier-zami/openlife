//
// Created by olivier on 07/08/2021.
//

#include "worldMap.h"

#include <iostream>

#include "OneLife/server/map.h"
#include "src/system/_base/object/store/device/random/linearDB.h"
#include "OneLife/server/dbCommon.h"

extern openLife::system::object::store::device::random::LinearDB *newBiomeDB;

/**
 *
 * @param width
 * @param height
 * @param detail
 */
server::component::database::WorldMap::WorldMap(unsigned int width, unsigned int height, unsigned int detail=4)
{
	this->width = width;
	this->height = height;
	this->center.x = (unsigned int)this->width/2;
	this->center.y = (unsigned int)this->height/2;
	this->idxMax = this->width*this->height;
	this->biome = std::vector<int>(this->idxMax);
	std::fill(this->biome.begin(), this->biome.end(), -1);
}

/**
 *
 */
server::component::database::WorldMap::~WorldMap() {}

/**
 *
 * @param posX
 * @param posY
 * @return
 */
server::component::database::WorldMap* server::component::database::WorldMap::select(int posX, int posY)
{
	unsigned int x = (unsigned)((signed)this->center.x+posX);
	if(x<0) x = 0;
	if(x>=this->width) x = this->width-1;
	this->query.x = x;

	unsigned int y = ((signed)this->center.y+posY);
	if(y<0) y = 0;
	if(y>=this->height) y = this->height-1;
	this->query.y = y;

	return this;
}

/**
 *
 * @param mapZone
 */
void server::component::database::WorldMap::insert(common::object::entity::MapZone* mapZone)
{
	unsigned int i;
	unsigned int x, y;
	struct{unsigned int x; unsigned int y;} targetCoord;
	for(i=0;i<mapZone->getSize();i++)
	{
		x = i%mapZone->getWidth();
		y = i/mapZone->getWidth();
		if((targetCoord.x=x+this->query.x)>=this->width)continue;
		if((targetCoord.y=y+this->query.y)>=this->height)continue;
		this->biome[targetCoord.x+(this->width*targetCoord.y)] = mapZone->p(i);
	}
}

/**
 *
 * @return
 */
int server::component::database::WorldMap::getBiome()
{
	unsigned int idx;
	idx = this->query.x+(this->query.y*this->width);
	return this->biome[idx];
}

void server::component::database::WorldMap::useBiomeStorehouse(openLife::system::object::store::device::random::LinearDB* biomeStoreHouse)
{
	this->biomeStoreHouse = biomeStoreHouse;
}

//!

void server::component::database::WorldMap::updateSecondPlaceIndex(int *outSecondPlaceIndex)
{
	this->tmp.outSecondPlaceIndex = outSecondPlaceIndex;
}

void server::component::database::WorldMap::updateSecondPlaceGap(double *outSecondPlaceGap)
{
	this->tmp.outSecondPlaceGap = outSecondPlaceGap;
}

int getMapBiomeIndex( int inX, int inY,
					  int *outSecondPlaceIndex,
					  double *outSecondPlaceGap)
					  {
	int pickedBiome;
	int secondPlaceBiome = -1;

	int dbBiome = -1;

	std::cout << "\nanyBiome ? " << anyBiomesInDB << " && ";
	std::cout << minBiomeXLoc << "<=" << inX << "<=" << maxBiomeXLoc << " && ";
	std::cout << minBiomeYLoc << "<=" << inY << "<=" << maxBiomeYLoc << "";

	if( anyBiomesInDB &&
	inX >= minBiomeXLoc && inX <= maxBiomeXLoc &&
	inY >= minBiomeYLoc && inY <= maxBiomeYLoc )
	{
		// don't bother with this call unless biome DB has
		// something in it, and this inX,inY is in the region where biomes
		// exist in the database (tutorial loading, or test maps)

		dbBiome = biomeDBGet( inX, inY,
							  &secondPlaceBiome,
							  outSecondPlaceGap );
		std::cout << "\n===================>"<<"get biome in db : " << dbBiome;
	}


	if( dbBiome != -1 )
	{

		int index = getBiomeIndex( dbBiome );

		if( index != -1 )
		{
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

	std::cout << "\nsecond place: " << secondPlace << ", secondePlaceGap: " << secondPlaceGap;

	pickedBiome = computeMapBiomeIndex( inX, inY,
										&secondPlace, &secondPlaceGap );

	std::cout << "\nPicked biome : "<<pickedBiome<<", second place: " << secondPlace << ", secondePlaceGap: " << secondPlaceGap;


	if( outSecondPlaceIndex != NULL ) {
		*outSecondPlaceIndex = secondPlace;
	}
	if( outSecondPlaceGap != NULL ) {
		*outSecondPlaceGap = secondPlaceGap;
	}


	if( dbBiome == -1 || secondPlaceBiome == -1 )
	{
		// not stored, OR some part of stored stale, re-store it

		secondPlaceBiome = 0;
		if( secondPlace != -1 ) {
			secondPlaceBiome = biomes[ secondPlace ];
		}

		// skip saving proc-genned biomes for now
		// huge RAM impact as players explore distant areas of map

		// we still check the biomeDB above for loading test maps
		//biomeDBPut( inX, inY, biomes[pickedBiome], secondPlaceBiome, secondPlaceGap );
	}


	int newBiome = worldMap->select(inX,inY)->getBiome();
	if(newBiome != -1) pickedBiome = newBiome;
	return pickedBiome;
}

/**
 *
 * @param inX
 * @param inY
 * @param outSecondPlaceBiome
 * @param outSecondPlaceGap
 * @return
 */
// returns -1 if not found
int biomeDBGet( int inX, int inY,
			  int *outSecondPlaceBiome,
			  double *outSecondPlaceGap)
{
	unsigned char key[8];
	unsigned char value[12];

	// look for changes to default in database
	intPairToKey( inX, inY, key );

	int result = newBiomeDB->get(key, value);
	//int result = LINEARDB3_get( &biomeDB, key, value );//TODO: search LINEARDB3_get ans replace with newBiomeDB->get(...)

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