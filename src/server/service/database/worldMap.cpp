//
// Created by olivier on 07/08/2021.
//

#include "worldMap.h"

#include <iostream>
#include <cmath>

#include "src/server/main.h"
#include "src/system/_base/object/store/device/random/linearDB.h"
#include "src/system/_base/process/scalar.h"

//!debug
#include "src/system/_base/object/process/handler/image.h"

//!legacy
#include "minorGems/util/log/AppLog.h"
#include "minorGems/io/file/File.h"
#include "src/system/_base/object/store/device/random/draft.h"
#include "minorGems/util/stringUtils.h"
#include "OneLife/server/dbCommon.h"
#include "OneLife/commonSource/fractalNoise.h"

extern openLife::server::service::database::WorldMap* worldMap;
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
extern char allowSecondPlaceBiomes;
extern int specialBiomeBandMode;
extern int numSpecialBiomes;
extern int regularBiomeLimit;
extern float *biomeCumuWeights;
extern float biomeTotalWeight;
extern int numBiomes;
extern unsigned int biomeRandSeedA;
extern unsigned int biomeRandSeedB;
extern openLife::system::object::store::memory::random::Biome* cachedBiome;
extern openLife::system::type::Value2D_U32 mapGenSeed;

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

	this->debugged = false;
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
void openLife::server::service::database::WorldMap::legacy(
		LINEARDB3* biomeDB,
		char* notEmptyDB,
		openLife::system::object::store::memory::random::Biome* dbCacheBiome
	)
{
	this->biomeDB = biomeDB;
	this->notEmptyDB = notEmptyDB;
	this->dbCacheBiome = dbCacheBiome;
}

/**
 *
 */
void openLife::server::service::database::WorldMap::debug()
{
	openLife::system::object::process::handler::Image* imageHandler;
	imageHandler = new openLife::system::object::process::handler::Image();
	imageHandler->load("/home/olivier/Projets/OpenLife/data/images/maps/mini_map.bmp");
	Image image = imageHandler->getImageInfo();
	std::cout << "\nModification image de dimension ("<<image.width<<","<<image.height<<")";
	imageHandler->clean();
	for(int y=-256; y<256; y++)
	{
		for(int x=-256; x<256; x++)
		{
			/*
			 int *outSecondPlaceIndex,
									  double *outSecondPlaceGap
			 */
			ColorRGB color;

			/**********************************************************************************************************/
			int pickedBiome = this->select(x, y)->getNewBiome().value;


			unsigned int offset = (!x || !y) ? 50 : 0;
			switch(pickedBiome)
			{
				case 0://swamp
				color = {95+offset, 0, 125+offset};
				break;
				case 1://grassland
				color = {0, 190+offset, 0};
				break;
				case 2://savannah
				color = {190+offset, 95+offset, 0};
				break;
				case 3://mountain
				color = {95+offset, 95+offset, 95+offset};
				break;
				case 4://jungle
				color = {0, 95+offset, 0};
				break;
				case 5://desert
				color = {190+offset, 190+offset, 0};
				break;
				case 6://polar
				color = {190+offset, 190+offset, 190+offset};
				break;
				default://unknown
				color = {0, 0, 255+offset};
				break;
			}

			int imgX = (image.width/2)+x;
			int imgY = (image.height/2)-y;
			if(imgX>=0&&imgX<512&&imgY>=0&&imgY<512) imageHandler->select(imgX, imgY)->setPixel(color);
			//imageHandler->select(x, y)->setPixel(color);
		}
	}
	imageHandler->save("/home/olivier/Projets/OpenLife/var/images/maps/game_starting_zone.bmp");
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

/**
 *
 * @return
 * @note topographic rings
 */
openLife::system::type::record::Biome openLife::server::service::database::WorldMap::getNewBiome()
{
	//!legacy : int computeMapBiomeIndex(x, y);
	openLife::system::type::record::Biome newBiome = {this->query.x, this->query.y, 0, -1, 0};

	// try topographical altitude mapping
	setXYRandomSeed( biomeRandSeedA, biomeRandSeedB );
	double randVal =( openLife::system::process::scalar::getXYFractal( this->query.x, this->query.y, 0.55, 0.83332 + 0.08333 * numBiomes, mapGenSeed ) );

	// push into range 0..1, based on sampled min/max values
	randVal -= 0.099668;
	randVal *= 1.268963;

	float i = randVal * biomeTotalWeight;
	while( newBiome.value < numBiomes && i > biomeCumuWeights[newBiome.value] ) newBiome.value++;

	if( newBiome.value >= numBiomes ) newBiome.value = numBiomes - 1;

	if( newBiome.value >= regularBiomeLimit && numSpecialBiomes > 0 )
	{
		// special case:  on a peak, place a special biome here
		if( specialBiomeBandMode )
		{
			// use band mode for these
			newBiome.value = getSpecialBiomeIndexForYBand( this->query.y );

			newBiome.secondPlace = regularBiomeLimit - 1;
			newBiome.secondPlaceGap = 0.1;
		}
		else
		{
			// use patches mode for these
			newBiome.value = -1;

			double maxValue = -10;
			double secondMaxVal = -10;

			for( int i=regularBiomeLimit; i<numBiomes; i++ )
			{
				int biome = biomes[i];
				setXYRandomSeed( biome * 263 + biomeRandSeedA + 38475, biomeRandSeedB );
				double randVal = openLife::system::process::scalar::getXYFractal(  this->query.x,
																				   this->query.y,
																				   0.55,
																				   2.4999 + 0.2499 * numSpecialBiomes,
																				   mapGenSeed );

				if( randVal > maxValue )
				{
					if( maxValue != -10 )
					{
						secondMaxVal = maxValue;
					}
					maxValue = randVal;
					newBiome.value = i;
				}
			}

			if( maxValue - secondMaxVal < 0.03 )
			{
				// close!  that means we're on a boundary between special biomes

				// stick last regular biome on this boundary, so special
				// biomes never touch
				newBiome.secondPlace = newBiome.value;
				newBiome.secondPlaceGap = 0.1;
				newBiome.value = regularBiomeLimit - 1;
			}
			else {
				newBiome.secondPlace = regularBiomeLimit - 1;
				newBiome.secondPlaceGap = 0.1;
			}
		}
	}
	else
	{
		// second place for regular biome rings

		newBiome.secondPlace = newBiome.value - 1;
		if( newBiome.secondPlace < 0 ) {
			newBiome.secondPlace = newBiome.value + 1;
		}
		newBiome.secondPlaceGap = 0.1;
	}


	if( ! allowSecondPlaceBiomes ) {
		// make the gap ridiculously big, so that second-place placement
		// never happens.
		// but keep secondPlace set different from pickedBiome
		// (elsewhere in code, we avoid placing animals if
		// secondPlace == picked
		newBiome.secondPlaceGap = 10.0;
	}

	return newBiome;
}

void openLife::server::service::database::WorldMap::insert(openLife::system::type::record::Biome biome)
{
	//!legacy biomeDBPut( int inX, int inY, int inValue, int inSecondPlace, double inSecondPlaceGap )
	unsigned char key[8];
	unsigned char value[12];

	intPairToKey( this->query.x, this->query.y, key );
	intToValue( biome.value, &( value[0] ) );
	intToValue( biome.secondPlace, &( value[4] ) );
	intToValue( lrint( biome.secondPlaceGap * gapIntScale ),
				&( value[8] ) );

	(*this->notEmptyDB) = true;

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
openLife::system::type::record::Biome openLife::server::service::database::WorldMap::getBiomeRecord()
{
	//!legacy int getMapBiomeIndex( int inX, int inY, int *outSecondPlaceIndex, double *outSecondPlaceGap)
	openLife::system::type::record::Biome biomeRecord = {this->query.x, this->query.y, -1, -1, 0};

	if( *(this->notEmptyDB) && this->query.x >= minBiomeXLoc && this->query.x <= maxBiomeXLoc && this->query.y >= minBiomeYLoc && this->query.y <= maxBiomeYLoc )
	{
		// don't bother with this call unless biome DB has
		// something in it, and this inX,inY is in the region where biomes
		// exist in the database (tutorial loading, or test maps)
		biomeRecord.value = biomeDBGet( this->query.x, this->query.y, &(biomeRecord.secondPlace), &(biomeRecord.secondPlaceGap) );
	}
	if( biomeRecord.value != -1 )
	{
		int index = getBiomeIndex( biomeRecord.value );
		if( index != -1 )
		{
			// biome still exists!
			char secondPlaceFailed = false;
			int secondIndex = getBiomeIndex( biomeRecord.secondPlace );

			if( secondIndex != -1 ) biomeRecord.secondPlace = secondIndex;
			else secondPlaceFailed = true;

			if(!secondPlaceFailed) return biomeRecord;//TODO: unique return principle
		}
		else biomeRecord.value = -1;

		// else a biome or second place in biome.db that isn't in game anymore
		// ignore it
	}

	biomeRecord = this->dbCacheBiome->get(this->query.x, this->query.y);
	if(biomeRecord.value == -1)
	{
		biomeRecord = this->getNewBiome();
		this->dbCacheBiome->put(biomeRecord);
	}


	if( biomeRecord.value == -1 || biomeRecord.secondPlace == -1 )
	{
		// not stored, OR some part of stored stale, re-store it
		biomeRecord.secondPlace = 0;
		if( biomeRecord.secondPlace != -1 ) biomeRecord.secondPlace = biomes[ biomeRecord.secondPlace ];
		// skip saving proc-genned biomes for now
		// huge RAM impact as players explore distant areas of map

		// we still check the biomeDB above for loading test maps
		//biomeDBPut( inX, inY, biomes[pickedBiome], secondPlaceBiome, secondPlaceGap );
	}
	return biomeRecord;
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