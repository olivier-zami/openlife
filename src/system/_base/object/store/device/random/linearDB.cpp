//
// Created by olivier on 09/08/2021.
//

#include "linearDB.h"

#include <iostream>
#include <cstring>
#include <cmath>

#include "src/system/_base/log.h"

#include "src/system/_base/process/hash/murmurhash.h"

//#include "src/common/process/hash.h"
#include "OneLife/server/lineardb3.h"
#include "OneLife/server/dbCommon.h"
#include "OneLife/server/map.h"


extern int maxBiomeXLoc;
extern int maxBiomeYLoc;
extern int minBiomeXLoc;
extern int minBiomeYLoc;
extern char lookTimeDBEmpty;
extern char skipLookTimeCleanup;

// if lookTimeDBEmpty, then we init all map cell look times to NOW
extern int cellsLookedAtToInit ;

LINEARDB3 lookTimeDB;
char lookTimeDBOpen  = false;
const double openLife::system::object::store::device::random::LinearDB::MAX_LOAD_FOR_OPEN_CALLS = 0.5;

/**
 *
 * @param settings
 */
openLife::system::object::store::device::random::LinearDB::LinearDB(openLife::system::settings::LinearDB settings)
{
	this->settings.filename = settings.filename;
	this->settings.mode = settings.mode;
	this->settings.hTableSize = settings.hTableSize;
	this->settings.record.keySize = settings.record.keySize;
	this->settings.record.valSize = settings.record.valSize;
	openLife::system::Log::notice("Create LinearDB file(%s)", this->settings.filename.c_str());
}

openLife::system::object::store::device::random::LinearDB::~LinearDB() {}

void openLife::system::object::store::device::random::LinearDB::handle(LINEARDB3 *db)
{
	this->db = db;
}


void openLife::system::object::store::device::random::LinearDB::set(int idx, int value) {}

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
 * private
 **/

/**
 *
 */
void openLife::system::object::store::device::random::LinearDB::createResource()
{
	this->file = fopen(this->filename.c_str(), "w+b");
}

//<<<<<<< Updated upstream
/*

int openLife::system::object::store::device::random::LinearDB::createResourceHeader()
{
	// returns 0 on success, -1 on error
	if(fseeko(this->file, 0, SEEK_SET)) return false;
	if(fwrite(this->magicString.c_str(), this->magicString.length(), 1, this->file) != 1) return false;
	if(fwrite(&(this->keySize), sizeof(uint32_t), 1, this->file ) != 1) return false;
	if(fwrite( &(this->valueSize), sizeof(uint32_t), 1, this->file) != 1) return false;
	return true;
}


uint64_t openLife::system::object::store::device::random::LinearDB::getBinNumber(uint32_t *outFingerprint)
{
	//uint32_t *outFingerprint;//fingerprint;

	//uint64_t getBinNumber( LINEARDB3 *inDB, const void *inKey, uint32_t *outFingerprint );*
	//{
		uint64_t hashVal = openLife::system::hash::MurmurHash(this->currentRecord.key, this->keySize, MURMURHASH64_LINEARDB_SEED1); //LINEARDB3_hash(inB, inLen)
		*outFingerprint = hashVal % this->fingerprintMod;//TODO: set fingerprintMod
		if( *outFingerprint == 0 )
		{
			// forbid straight 0 as fingerprint value
			// we use 0-fingerprints as not-present flags
			// for the rare values that land on 0, we need to make sure
			// main hash changes along with fingerprint

			if( hashVal < UINT64_MAX )
			{
				hashVal++;
			}
			else
			{
				hashVal--;
			}
			*outFingerprint = hashVal % this->fingerprintMod;
		}
		//return getBinNumberFromHash( inDB, hashVal );
	//}

	//uint64_t getBinNumberFromHash( LINEARDB3 *inDB, uint64_t inHashVal );
	//{
		uint64_t inHashVal = hashVal;
		uint64_t binNumberA = inHashVal % (uint64_t)( this->hashTableSizeA );
		uint64_t binNumberB = binNumberA;
		unsigned int splitPoint = this->hashTableSizeB - this->hashTableSizeA;
		if( binNumberA < splitPoint )
		{
			// points before split can be mod'ed with double base table size

			// binNumberB will always fit in hashTableSizeB, the expanded table
			binNumberB = inHashVal % (uint64_t)( this->hashTableSizeA * 2 );
		}
		return binNumberB;
	//}
}
*/

uint32_t openLife::system::object::store::device::random::LinearDB::getComputedTableSize(uint32_t inNumRecords)
{
	uint32_t minTableRecords = (uint32_t)ceil( inNumRecords / this->maxLoad );

	uint32_t minTableBuckets =(uint32_t)ceil( (double)minTableRecords /
			(double)RECORDS_PER_BUCKET );
//=======
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
//>>>>>>> Stashed changes

	// even if file contains no inserted records
	// use 2 buckets minimum
	if( minTableBuckets < 2 ) {
		minTableBuckets = 2;
	}
	return minTableBuckets;
}

/**
 *
 */
void openLife::system::object::store::device::random::LinearDB::recomputeFingerprintMod()
{
	//printf("\n====> recomputeFingerprintMod : %i => %i", this->fingerprintMod, this->hashTableSizeA);
	this->fingerprintMod = this->hashTableSizeA;

	while( true )
	{
		uint32_t newMod = this->fingerprintMod * 2;
		if( newMod <= this->fingerprintMod )
		{
			// reached 32-bit limit
			return;
		}
		else
		{
			//printf("\n====> recomputeFingerprintMod : %i => %i", this->fingerprintMod, newMod);
			this->fingerprintMod = newMod;
		}
	}
}

/**********************************************************************************************************************/

void dbLookTimePut( int inX, int inY, timeSec_t inTime )
{
	if( !lookTimeDBOpen ) return;

	unsigned char key[8];
	unsigned char value[8];


	intPairToKey( inX/100, inY/100, key );
	timeToValue( inTime, value );


	LINEARDB3_put( &lookTimeDB, key, value );
}

// returns 0 if not found
timeSec_t dbLookTimeGet( int inX, int inY )
{
	unsigned char key[8];
	unsigned char value[8];

	intPairToKey( inX/100, inY/100, key );

	int result = LINEARDB3_get( &lookTimeDB, key, value );//DB_get

	if( result == 0 ) {
		// found
		return valueToTime( value );
	}
	else {
		return 0;
	}
}
