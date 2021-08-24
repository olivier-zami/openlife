//
// Created by olivier on 07/08/2021.
//

#include "worldMap.h"

#include <iostream>
#include <cmath>

#include "src/system/_base/object/store/device/random/linearDB.h"

//!legacy
#include "minorGems/util/log/AppLog.h"
#include "minorGems/io/file/File.h"
#include "src/system/_base/object/store/device/random/draft.h"
#include "OneLife/server/map.h"
#include "minorGems/util/stringUtils.h"
#include "OneLife/server/dbCommon.h"

extern LINEARDB3 biomeDB;
extern char anyBiomesInDB;
extern int maxBiomeXLoc;
extern int maxBiomeYLoc;
extern int minBiomeXLoc;
extern int minBiomeYLoc;
extern openLife::system::object::store::device::random::LinearDB *newBiomeDB;
extern char lookTimeDBEmpty;
extern char skipLookTimeCleanup;
extern int cellsLookedAtToInit;
extern double gapIntScale;

/**
 *
 * @param width
 * @param height
 * @param detail
 */
openLife::server::service::database::WorldMap::WorldMap(openLife::system::settings::database::WorldMap settings/*unsigned int width, unsigned int height, unsigned int detail=4*/)
{
	this->width = settings.mapSize.width;
	this->height = settings.mapSize.height;
	this->center.x = (unsigned int)this->width/2;
	this->center.y = (unsigned int)this->height/2;
	this->idxMax = this->width*this->height;
	this->biome = std::vector<int>(this->idxMax);
	std::fill(this->biome.begin(), this->biome.end(), -1);

	this->settings = settings;
}

/**
 *
 */
openLife::server::service::database::WorldMap::~WorldMap() {}

/**
 *
 * @param biomeDB
 * @note temporary methods
 */
void openLife::server::service::database::WorldMap::handleBiomeDB(LINEARDB3* biomeDB)
{
	this->biomeDB = biomeDB;
}

/**
 *
 */
int openLife::server::service::database::WorldMap::init()
{
	//!legacy => DB_open_timeShrunk(db=>this->biomeDB, path=>s.filename, mode=>KISSDB_OPEN_MODE_RWCREAT, hash_table_size=>80000, key_size=>8, value_size=>12)
	/*
    error = DB_open_timeShrunk( &biomeDB,
                         "biome.db",
                         KISSDB_OPEN_MODE_RWCREAT,
                         80000,
                         8, // two 32-bit ints, xy
                         12 // three ints,
                         // 1: biome number at x,y
                         // 2: second place biome number at x,y
                         // 3: second place biome gap as int (float gap
                         //    multiplied by 1,000,000)
                         );
                         */
	const char* path = this->settings.filename.c_str();
	int mode = 3;//#define KISSDB_OPEN_MODE_RWCREAT 3
	unsigned long hash_table_size = 80000;
	unsigned long key_size = 8;
	unsigned long value_size = 12;

	File dbFile( NULL, path);

	if( ! dbFile.exists() || lookTimeDBEmpty || skipLookTimeCleanup )
	{

		if( lookTimeDBEmpty ) AppLog::infoF( "No lookTimes present, not cleaning %s", path );//TODO: use loginServiceInstance

		int error = LINEARDB3_open( this->biomeDB,
									path,
									mode,
									hash_table_size,
									key_size,
									value_size );

		if( ! error && ! skipLookTimeCleanup )
		{
			// add look time for cells in this DB to present
			// essentially resetting all look times to NOW
			LINEARDB3_Iterator dbi;
			LINEARDB3_Iterator_init( this->biomeDB, &dbi );
			// key and value size that are big enough to handle all of our DB
			unsigned char key[16];
			unsigned char value[12];

			while( LINEARDB3_Iterator_next( &dbi, key, value ) > 0 )
			{
				int x = valueToInt( key );
				int y = valueToInt( &( key[4] ) );
				cellsLookedAtToInit++;
				dbLookTimePut( x, y, Time::timeSec() );
			}
		}
		return error;
	}

	char *dbTempName = autoSprintf( "%s.temp", path );
	File dbTempFile( NULL, dbTempName );

	if( dbTempFile.exists() )
	{
		dbTempFile.remove();
	}

	if( dbTempFile.exists() )
	{
		AppLog::errorF( "Failed to remove temp DB file %s", dbTempName );
		delete [] dbTempName;
		return LINEARDB3_open( this->biomeDB,
							   path,
							   mode,
							   hash_table_size,
							   key_size,
							   value_size );
	}

	LINEARDB3 oldDB;
	int error = LINEARDB3_open( &oldDB,
								path,
								mode,
								hash_table_size,
								key_size,
								value_size );
	if( error )
	{
		AppLog::errorF( "Failed to open DB file %s in DB_open_timeShrunk", path );
		delete [] dbTempName;
		return error;
	}

	LINEARDB3_Iterator dbi;
	LINEARDB3_Iterator_init( &oldDB, &dbi );

	// key and value size that are big enough to handle all of our DB
	unsigned char key[16];
	unsigned char value[12];

	int total = 0;
	int stale = 0;
	int nonStale = 0;

	// first, just count
	while( LINEARDB3_Iterator_next( &dbi, key, value ) > 0 )
	{
		total++;

		int x = valueToInt( key );
		int y = valueToInt( &( key[4] ) );

		if( dbLookTimeGet( x, y ) > 0 )
		{
			// keep
			nonStale++;
		}
		else {
			// stale
			// ignore
			stale++;
		}
	}

	// optimial size for DB of remaining elements
	unsigned int newSize = LINEARDB3_getShrinkSize( &oldDB, nonStale );

	AppLog::infoF( "Shrinking hash table in %s from %d down to %d",
				   path,
				   LINEARDB3_getCurrentSize( &oldDB ),
				   newSize );


	LINEARDB3 tempDB;

	error = LINEARDB3_open( &tempDB,
							dbTempName,
							mode,
							newSize,
							key_size,
							value_size );
	if( error ) {
		AppLog::errorF( "Failed to open DB file %s in DB_open_timeShrunk",
						dbTempName );
		delete [] dbTempName;
		LINEARDB3_close( &oldDB );
		return error;
	}


	// now that we have new temp db properly sized,
	// iterate again and insert, but don't count
	LINEARDB3_Iterator_init( &oldDB, &dbi );

	while( LINEARDB3_Iterator_next( &dbi, key, value ) > 0 ) {
		int x = valueToInt( key );
		int y = valueToInt( &( key[4] ) );

		if( dbLookTimeGet( x, y ) > 0 ) {
			// keep
			// insert it in temp
			LINEARDB3_put( &tempDB, key, value );
		}
		else {
			// stale
			// ignore
		}
	}



	AppLog::infoF( "Cleaned %d / %d stale map cells from %s", stale, total,
				   path );

	printf( "\n" );


	LINEARDB3_close( &tempDB );
	LINEARDB3_close( &oldDB );

	dbTempFile.copy( &dbFile );
	dbTempFile.remove();

	delete [] dbTempName;

	// now open new, shrunk file
	return LINEARDB3_open( this->biomeDB,
						   path,
						   mode,
						   hash_table_size,
						   key_size,
						   value_size );
	/***/
}

/**
 *
 * @param posX
 * @param posY
 * @return
 */
openLife::server::service::database::WorldMap* openLife::server::service::database::WorldMap::select(int posX, int posY)
{
	this->query.x = posX;
	this->query.y = posY;
	/*
	unsigned int x = (unsigned)((signed)this->center.x+posX);
	if(x<0) x = 0;
	if(x>=this->width) x = this->width-1;
	this->query.x = x;

	unsigned int y = ((signed)this->center.y+posY);
	if(y<0) y = 0;
	if(y>=this->height) y = this->height-1;
	this->query.y = y;
	*/
	return this;
}

void openLife::server::service::database::WorldMap::insert(openlife::system::type::record::Biome biome)
{
	//!legacy biomeDBPut( int inX, int inY, int inValue, int inSecondPlace, double inSecondPlaceGap )
	unsigned char key[8];
	unsigned char value[12];

	intPairToKey( this->query.x, this->query.y, key );
	intToValue( biome.value, &( value[0] ) );
	intToValue( biome.secondPlace, &( value[4] ) );
	intToValue( lrint( biome.secondPlaceGap * gapIntScale ),
				&( value[8] ) );

	anyBiomesInDB = true;

	if( this->query.x > maxBiomeXLoc ) {
		maxBiomeXLoc = this->query.x;
	}
	if( this->query.x < minBiomeXLoc ) {
		minBiomeXLoc = this->query.x;
	}
	if( this->query.y > maxBiomeYLoc ) {
		maxBiomeYLoc = this->query.y;
	}
	if( this->query.y < minBiomeYLoc ) {
		minBiomeYLoc = this->query.y;
	}

	LINEARDB3_put(biomeDB, key, value);
}

/**
 *
 * @param mapZone
 */
void openLife::server::service::database::WorldMap::insert(common::object::entity::MapZone* mapZone)
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
int openLife::server::service::database::WorldMap::getBiome()
{
	unsigned int idx;
	idx = this->query.x+(this->query.y*this->width);
	return this->biome[idx];
}

void openLife::server::service::database::WorldMap::useBiomeStorehouse(openLife::system::object::store::device::random::LinearDB* biomeStoreHouse)
{
	this->biomeStoreHouse = biomeStoreHouse;
}

//!

void openLife::server::service::database::WorldMap::updateSecondPlaceIndex(int *outSecondPlaceIndex)
{
	this->tmp.outSecondPlaceIndex = outSecondPlaceIndex;
}

void openLife::server::service::database::WorldMap::updateSecondPlaceGap(double *outSecondPlaceGap)
{
	this->tmp.outSecondPlaceGap = outSecondPlaceGap;
}

extern openLife::server::service::database::WorldMap* worldMap;

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


	//int newBiome = worldMap->select(inX,inY)->getBiome();
	//if(newBiome != -1) pickedBiome = newBiome;
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

	//int result = newBiomeDB->get(key);
	int result = LINEARDB3_get( &biomeDB, key, value );//TODO: search LINEARDB3_get ans replace with newBiomeDB->get(...)

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