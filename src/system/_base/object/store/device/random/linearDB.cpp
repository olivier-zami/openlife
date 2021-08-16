//
// Created by olivier on 09/08/2021.
//

#include "linearDB.h"

#include <iostream>
#include <cstring>
#include <cmath>

#include "src/system/_base/hash/murmurhash.h"

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

const double openLife::system::object::store::device::random::LinearDB::MAX_LOAD_FOR_OPEN_CALLS = 0.5;

/**
 *
 * @param settings
 */
openLife::system::object::store::device::random::LinearDB::LinearDB(openLife::system::settings::LinearDB settings)
{
	this->filename = settings.filename;
	this->keySize = settings.record.keySize;
	this->valueSize = settings.record.valueSize;
	this->recordBuffer = nullptr;
	this->maxOverflowDepth = 0;
	this->numRecords = 0;
	this->maxLoad = MAX_LOAD_FOR_OPEN_CALLS;
	if(!this->isResourceExist())
	{

	}
}

openLife::system::object::store::device::random::LinearDB::~LinearDB() {}

/**
 *
 * @param idx
 * @param value
 * @return
 */
int openLife::system::object::store::device::random::LinearDB::get(void* key)
{
	std::cout << "\ncall LinearDB::get",
	std::cout << "\nget value from " << this->filename << "";
	//uint64_t hashVal = openLife::system::hash::MurmurHash(inKey, inDB->keySize, MURMURHASH64_LINEARDB_SEED1);
	/*
	 => int biomeDBGet( int inX, int inY, int *outSecondPlaceBiome, double *outSecondPlaceGap)
	 	=> int LINEARDB3_get( LINEARDB3 *inDB, const void *inKey, void *outValue );
	 		=> int LINEARDB3_getOrPut( LINEARDB3 *inDB, const void *inKey, void *inOutValue, char inPut, char inIgnoreDataFile = false );
	 			=> getBinNumber( inDB, inKey, &fingerprint );
	 */
	return 0;
}

void openLife::system::object::store::device::random::LinearDB::put(int idx, int value) {}

int openLife::system::object::store::device::random::LinearDB::isResourceExist()
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
void openLife::system::object::store::device::random::LinearDB::init(LINEARDB3 *dbState)
{
	this->dbState = dbState;
}

/**
 *
 */
void openLife::system::object::store::device::random::LinearDB::createResource()
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
/****/
#if !defined(OPENLIFE_UNIT_TEST)

#endif
/****/