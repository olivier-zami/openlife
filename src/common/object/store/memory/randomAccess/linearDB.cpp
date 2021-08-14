//
// Created by olivier on 09/08/2021.
//

#include "linearDB.h"

#include <iostream>
#include <cstring>
#include <cmath>

#include "src/common/process/hash.h"
#include "OneLife/server/lineardb3.h"
#include "OneLife/server/dbCommon.h"
#include "OneLife/server/map.h"


extern char anyBiomesInDB;
extern int maxBiomeXLoc;
extern int maxBiomeYLoc;
extern int minBiomeXLoc;
extern int minBiomeYLoc;
extern char lookTimeDBEmpty;
extern char skipLookTimeCleanup;

// if lookTimeDBEmpty, then we init all map cell look times to NOW
extern int cellsLookedAtToInit ;

const double common::object::store::memory::randomAccess::LinearDB::MAX_LOAD_FOR_OPEN_CALLS = 0.5;

/**
 *
 * @param settings
 */
common::object::store::memory::randomAccess::LinearDB::LinearDB(common::type::settings::LinearDB settings)
{
	this->filename = settings.filename;
	this->recordBuffer = nullptr;
	this->maxOverflowDepth = 0;
	this->numRecords = 0;
	this->maxLoad = MAX_LOAD_FOR_OPEN_CALLS;
	if(!this->isResourceExist())
	{

	}
}

common::object::store::memory::randomAccess::LinearDB::~LinearDB() {}

/**
 *
 * @param idx
 * @param value
 * @return
 */
int common::object::store::memory::randomAccess::LinearDB::get(unsigned char idx[8], unsigned char value[12])
{
	return LINEARDB3_get(this->dbState, idx, value);
}

void common::object::store::memory::randomAccess::LinearDB::put(int idx, int value) {}

int common::object::store::memory::randomAccess::LinearDB::isResourceExist()
{
	int response = false;
	if((this->file = fopen(this->filename.c_str(), "r")))
	{
		fclose(this->file);
		response = true;
	}
	return response;
}

/**
 *
 * @param dbState
 */
void common::object::store::memory::randomAccess::LinearDB::init(LINEARDB3 *dbState)
{
	this->dbState = dbState;
}

/**
 *
 */
void common::object::store::memory::randomAccess::LinearDB::createResource()
{
	this->file = fopen(this->filename.c_str(), "w+b");
}

/*
DB_open_timeShrunk( &biomeDB,
                         "biome.db",
                         KISSDB_OPEN_MODE_RWCREAT,
                         80000,
                         8, // two 32-bit ints, xy
                         12 // three ints,
                         // 1: biome number at x,y
                         // 2: second place biome number at x,y
                         // 3: second place biome gap as int (float gap
                         //    multiplied by 1,000,000)
                         )
 */
// version of open call that checks whether look time exists in lookTimeDB
// for each record in opened DB, and clears any entries that are not
// rebuilding file storage for DB in the process
// lookTimeDB MUST be open before calling this
//
// If lookTimeDBEmpty, this call just opens the target DB normally without
// shrinking it.
//
// Can handle max key and value size of 16 and 12 bytes
// Assumes that first 8 bytes of key are xy as 32-bit ints
int DB_open_timeShrunk(
		LINEARDB3* db,
		const char *path,
		int mode,
		unsigned long hash_table_size,
		unsigned long key_size,
		unsigned long value_size)
{
	File dbFile( NULL, path );

	if( ! dbFile.exists() || lookTimeDBEmpty || skipLookTimeCleanup )
	{

		if( lookTimeDBEmpty ) {
			AppLog::infoF( "No lookTimes present, not cleaning %s", path );
		}

		int error = LINEARDB3_open( db,
							 path,
							 mode,
							 hash_table_size,
							 key_size,
							 value_size );

		if( ! error && ! skipLookTimeCleanup ) {
			// add look time for cells in this DB to present
			// essentially resetting all look times to NOW

			LINEARDB3_Iterator dbi;


			LINEARDB3_Iterator_init( db, &dbi );

			// key and value size that are big enough to handle all of our DB
			unsigned char key[16];

			unsigned char value[12];

			while( LINEARDB3_Iterator_next( &dbi, key, value ) > 0 ) {
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

	if( dbTempFile.exists() ) {
		dbTempFile.remove();
	}

	if( dbTempFile.exists() ) {
		AppLog::errorF( "Failed to remove temp DB file %s", dbTempName );

		delete [] dbTempName;

		return LINEARDB3_open( db,
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
	if( error ) {
		AppLog::errorF( "Failed to open DB file %s in DB_open_timeShrunk",
						path );
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
	while( LINEARDB3_Iterator_next( &dbi, key, value ) > 0 ) {
		total++;

		int x = valueToInt( key );
		int y = valueToInt( &( key[4] ) );

		if( dbLookTimeGet( x, y ) > 0 ) {
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
	return LINEARDB3_open( db,
					path,
					mode,
					hash_table_size,
					key_size,
					value_size );
}
