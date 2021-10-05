//
// Created by olivier on 07/08/2021.
//

#include "worldMap.h"

#include <iostream>
#include <cmath>
#include <cstdio>

#include "src/server/main.h"
#include "src/system/_base/log.h"
#include "src/system/_base/object/store/device/random/linearDB.h"
#include "src/system/_base/process/scalar.h"
#include "src/server/process/mapGenerator/newBiome_v0.h"
#include "src/server/process/newBiome_v1.h"

//!debug
#include "src/system/_base/object/process/handler/image.h"

//!legacy
#include "src/server/main.h" //TODO: set cachesize value in conf file
//#include "src/third_party/jason_rohrer/minorGems/util/log/AppLog.h" //TODO: use openLife::system::Log::trace(...);
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
extern openLife::system::type::Value2D_U32 mapGenSeed;



/**
 *
 * @param width
 * @param height
 * @param detail
 */
openLife::server::service::database::WorldMap::WorldMap(openLife::server::settings::WorldMap settings/*unsigned int width, unsigned int height, unsigned int detail=4*/)
{
	this->mapGenerator.type = (int)settings.mapGenerator.type;
	this->mapGenerator.sketch.filename = settings.mapGenerator.sketch.filename;
	this->width = settings.mapSize.width;
	this->height = settings.mapSize.height;
	this->center.x = (unsigned int)this->width/2;
	this->center.y = (unsigned int)this->height/2;
	this->idxMax = this->width*this->height;
	this->map.specialBiomeBandMode = settings.specialBiomeBandMode;
	this->map.seed.x = settings.map.seed.x;
	this->map.seed.y = settings.map.seed.y;
	this->map.allowSecondPlaceBiomes = allowSecondPlaceBiomes;
	this->map.specialBiomeBandThickness = settings.map.specialBiomeBandThickness;

	this->mappedBiomeValue.reserve(settings.biome1.size() + 1);
	for(unsigned int i=0; i<settings.biome1.size(); i++)//TODO: rename biome1 to biome after settings struct modification
	{
		this->mappedBiomeValue[settings.biome1[i].value] = settings.biome1[i].code;
		this->biome.push_back(settings.biome1[i].value);
	}

	for(unsigned int i=0; i<settings.map.specialBiomeBandYCenter.size(); i++)
	{
		this->map.specialBiomeBandYCenter.push_back(settings.map.specialBiomeBandYCenter[i]);
	}
	for(unsigned int i=0; i<settings.map.specialBiomes.size(); i++)
	{
		this->map.specialBiomes.push_back(settings.map.specialBiomes[i]);
	}
	for(unsigned int i=0; i<settings.map.biomeWeight.size(); i++)
	{
		this->map.biomeWeight.push_back(settings.map.biomeWeight[i]);
	}
	for(unsigned int i=0; i<settings.map.specialBiomeBandOrder.size(); i++)
	{
		this->map.specialBiomeBandOrder.push_back(settings.map.specialBiomeBandOrder[i]);
	}

	/*
	this->biome.reserve(settings.climate.size());//TODO: climate.size() must be the same size of biomeOrder
	std::cout << "\nregister " << settings.climate.size() << " biomes in size " << this->biome.capacity();
	for(unsigned int i=0; i<settings.climate.size(); i++)
	{
		openLife::server::service::database::worldMap::Biome dataBiome;
		dataBiome.label = settings.climate[i].label;
		this->biome.push_back(dataBiome);
	}
 	*/

	for(unsigned int i=0; i<settings.biome.order.size(); i++) this->dataBiome.order.push_back(settings.biome.order[i]);

	this->mapTile = std::vector<int>(this->idxMax);
	std::fill(this->mapTile.begin(), this->mapTile.end(), -1);
	
	this->dbCacheBiome = new openLife::system::object::store::memory::random::Biome(BIOME_CACHE_SIZE);
	this->dbBiomeCache = new openLife::system::object::store::memory::ExtendedVector2D<openLife::system::type::entity::Biome>();

	//!
	this->map.relief = settings.relief;

	//!
	openLife::system::settings::LinearDB mapDBSettings; //TODO: take value from config
	mapDBSettings.filename = "map.db";
	mapDBSettings.mode = KISSDB_OPEN_MODE_RWCREAT;
	mapDBSettings.hTableSize = 80000;
	mapDBSettings.record.keySize = 16;
	mapDBSettings.record.valSize = 4;
	this->mapDB = new openLife::system::object::store::device::random::LinearDB(mapDBSettings);
}

/**
 *
 */
openLife::server::service::database::WorldMap::~WorldMap() {}

/**
 *
 */
int openLife::server::service::database::WorldMap::init()//TODO: put code in constructor
{
	//!legacy => DB_open_timeShrunk(db=>this->biomeDB, path=>s.filename, mode=>KISSDB_OPEN_MODE_RWCREAT, hash_table_size=>80000, key_size=>8, value_size=>12)
	//error = DB_open_timeShrunk( &biomeDB,
	//"biome.db",
	//KISSDB_OPEN_MODE_RWCREAT,
	//80000,
	//8, // two 32-bit ints, xy
	//12 // three ints,
	// 1: biome number at x,y
	// 2: second place biome number at x,y
	// 3: second place biome gap as int (float gap
	//    multiplied by 1,000,000)
	//);

	/*
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
   */
	return 0;
}

/**
 *
 * @param x
 * @param y
 */
void openLife::server::service::database::WorldMap::setMapSeed(unsigned int x, unsigned int y)
{
	this->map.seed.x = x;
	this->map.seed.y = y;
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
	return this;
}

/**
 *
 * @return
 * @note topographic rings
 */
openLife::system::type::entity::Biome openLife::server::service::database::WorldMap::getNewBiome()
{
	openLife::system::type::entity::Biome newGround;
	switch(this->mapGenerator.type)
	{
		case 0:
			newGround = {this->query.x, this->query.y, 1, 0, 0};
			break;
		case 1:
			newGround = openLife::server::process::newBiome_v0(
					{this->query.x, this->query.y},
					{-256, -256},
					this->mapGenerator.sketch.filename,
					this->dbBiomeCache);
			break;
		case 2:
			newGround = openLife::server::process::newBiome_v1(
				this->query.x,
				this->query.y,
				this->map.seed,
				this->map.allowSecondPlaceBiomes,
				this->dataBiome.order,
				this->map.specialBiomeBandThickness,
				this->map.specialBiomeBandYCenter,
				this->map.specialBiomeBandOrder,
				this->map.specialBiomeBandMode,
				this->map.specialBiomes,
				this->map.biomeWeight);
			break;
		case 3:
			newGround = {this->query.x, this->query.y, -1, 0, 0};
			break;
	}
	return newGround;
}

std::vector<int> openLife::server::service::database::WorldMap::getBiomes()
{
	return this->biome;
}

int openLife::server::service::database::WorldMap::getInfoBiome(int biome)
{
	//printf("\n==========>getInfoBiome(%i)", biome);
	return this->biome[biome];
}


void openLife::server::service::database::WorldMap::insert(openLife::system::type::entity::Biome biome)
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

	if(false)
	{
		LINEARDB3_put(&biomeDB, key, value);//TODO: divide by zero bug must corrected
	}
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
		this->mapTile[targetCoord.x+(this->width*targetCoord.y)] = mapZone->p(i);
	}
}

int openLife::server::service::database::WorldMap::get()
{
	openLife::system::Log::trace("Read mapTile(%i,%i)", this->query.x, this->query.y);
	return 0;
}


/**
 *
 * @return
 */
openLife::system::type::entity::Biome openLife::server::service::database::WorldMap::getBiomeRecord(char forceValue)
{
	//!legacy int getMapBiomeIndex( int inX, int inY, int *outSecondPlaceIndex, double *outSecondPlaceGap)
	openLife::system::type::entity::Biome biomeRecord = {this->query.x, this->query.y, -1, -1, 0};

	if( /* *(this->notEmptyDB) &&*/ this->query.x >= minBiomeXLoc && this->query.x <= maxBiomeXLoc && this->query.y >= minBiomeYLoc && this->query.y <= maxBiomeYLoc )
	{
		// don't bother with this call unless biome DB has
		// something in it, and this inX,inY is in the region where biomes
		// exist in the database (tutorial loading, or test maps)

		//!legacy biomeRecord.value = biomeDBGet( this->query.x, this->query.y, &(biomeRecord.secondPlace), &(biomeRecord.secondPlaceGap) );
		/*******************************/
		unsigned char key[8];
		unsigned char value[12];

		// look for changes to default in database
		intPairToKey( this->query.x, this->query.y, key );

		//int result = LINEARDB3_get( &biomeDB, key, value );//TODO: search LINEARDB3_get ans replace with newBiomeDB->get(...) & may cause divide by zedo exception if some biome added
		int result = -1;

		if( result == 0 )
		{
			// found
			biomeRecord.value = valueToInt( &( value[0] ) );
			biomeRecord.secondPlace = valueToInt( &( value[4] ) );
			biomeRecord.secondPlaceGap = valueToInt( &( value[8] ) ) / gapIntScale;
		}
		else {
			biomeRecord.value = -1;
		}
		/********************/
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
	else
	{
		if(forceValue)
		{
			biomeRecord = this->dbCacheBiome->get(this->query.x, this->query.y);
			if(biomeRecord.value == -1)
			{
				biomeRecord = this->getNewBiome();
				this->dbCacheBiome->put(biomeRecord);
			}
		}
	}

	if( biomeRecord.value == -1 || biomeRecord.secondPlace == -1 )
	{
		// not stored, OR some part of stored stale, re-store it
		biomeRecord.secondPlace = 0;
		if( biomeRecord.secondPlace != -1 ) biomeRecord.secondPlace = this->getBiomes()[biomeRecord.secondPlace]/*biomes[biomeRecord.secondPlace]*/;
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
	//unsigned int idx;
	//idx = this->query.x+(this->query.y*this->width);
	//int relief = this->mapTile[idx];
	int relief = this->select(this->query.x, this->query.y)->getBiomeRecord().value;
	//printf("\n=====>Retrieve biome value from worldMap(%i, %i) : nbrBiome=%lu (rawValue=%i, mappedValue=%i)\n", this->query.x, this->query.y, this->mappedBiomeValue.size(), relief, this->mappedBiomeValue[relief]);
	return this->mappedBiomeValue[relief];
}

/**
 *
 * @return self
 */
openLife::server::service::database::WorldMap *openLife::server::service::database::WorldMap::reset()
{
	openLife::system::Log::trace("erasing (%i, %i)", this->query.x, this->query.y);
	return this;
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
