//
// Created by olivier on 09/08/2021.
//

#include "linearDB.h"

#include <iostream>
#include <cstring>
#include <cmath>

#include "src/system/_base/object/entity/exception.h"
#include "src/system/_base/file.h"
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
const int openLife::system::object::store::device::random::LinearDB::HEADER_SIZE = 11;

openLife::system::object::entity::Exception exception;

/**
 *
 * @param settings
 */
openLife::system::object::store::device::random::LinearDB::LinearDB(openLife::system::settings::LinearDB settings)
{
	this->filename = settings.filename;
	this->magicString = settings.magicString;
	this->keySize = settings.record.keySize; //inKeySize
	this->valueSize = settings.record.valueSize; //inValueSize
	this->recordBuffer = nullptr; //inDB->recordBuffer = NULL;
	this->maxOverflowDepth = 0; //inDB->maxOverflowDepth = 0;
	this->numRecords = 0; //inDB->numRecords = 0;
	this->maxLoad = MAX_LOAD_FOR_OPEN_CALLS; //inDB->maxLoad = maxLoadForOpenCalls;

	if(!this->isResourceExist())
	{
		openLife::system::File::create(settings.filename.c_str());
	}
	if(!(this->file = fopen(this->filename.c_str(), "w+b"))) throw exception("db file creation failed");//this->file = inDB->file

	unsigned int hashTableSize = (settings.hashTable.size < 2) ? 2 : settings.hashTable.size;
	this->hashTableSizeA = hashTableSize;
	this->hashTableSizeB = hashTableSize;

	this->recordSizeBytes = this->keySize + this->valueSize;// first byte in record is present flag
	this->recordBuffer = new uint8_t[this->recordSizeBytes];

	std::cout << "\ninHashTableStartSize = " << hashTableSize << " is created";
	this->fingerprintMod = this->hashTableSizeA;

	//test reached 32-bit limit
	for(uint32_t newMod=this->fingerprintMod*2;newMod>this->fingerprintMod;newMod=this->fingerprintMod*2)this->fingerprintMod=newMod;//recomputeFingerprintMod();

	this->hashTable = new openLife::system::object::store::device::random::linearDB::PageManager(); // new PageManager() inPM
	this->overflowBuckets = new openLife::system::object::store::device::random::linearDB::PageManager(); //new PageManager() inPM

	// does the file already contain a header

	// seek to the end to find out file size
	if(fseeko(this->file, 0, SEEK_END )) throw exception("end of resource file non reachable");

	if(ftello(this->file) < HEADER_SIZE)
	{
		// file that doesn't even contain the header
		// write fresh header and hash table
		// rewrite header
		if(!this->createResourceHeader()) throw exception("Fail to create Header in ...");
		this->lastOp = opWrite;

		this->hashTable->init(this->hashTableSizeA);
		this->overflowBuckets->init(2);
	}
	else
	{
		/*isFileHeaderValid()*/
		// read header
		if( fseeko(this->file, 0, SEEK_SET)) throw exception("Fail to read Header in ...");
		int numRead;
		char magicBuffer[ 4 ];
		numRead = fread(magicBuffer, 3, 1, this->file );
		if( numRead != 1 ) throw exception("Fail to read magic string in dbFile...");
	   	magicBuffer[3] = '\0';
	   	/*end isFileHeaderValid()*/
	   	if(strcmp(magicBuffer, this->magicString.c_str()) != 0)throw exception("lineardb3 magic string '%s' not found at start of file header in %s"/*this->magicString, this->filename*/);

	   	uint32_t val32;
	   	numRead = fread(&val32, sizeof(uint32_t), 1, this->file);
	   	if( numRead != 1 )throw exception("Failed to read key size"/*this->magicString, this->inPath*/);
	   	if( val32 != this->keySize ) throw exception("Requested lineardb3 key size of %u does not match size of %u in file header in %s"/*this->keySize, val32, this->filename*/);

	   	numRead = fread(&val32, sizeof(uint32_t), 1, this->file);
	   	if( numRead != 1 )throw exception("Failed to read value size"/*this->magicString, this->inPath*/);
	   	if( val32 != this->valueSize ) throw exception("Requested lineardb3 value size of %u does not match size of %u in file header in %s"/*this->keyValue, val32, this->filename*/);

	   	// got here, header matches

	   	// make sure hash table exists in file
	   	if( fseeko( this->file, 0, SEEK_END ) ) throw exception("Failed to find the hashtable");
		uint64_t fileSize = ftello( this->file );
		uint64_t numRecordsInFile = ( fileSize - HEADER_SIZE ) / this->recordSizeBytes;
		uint64_t expectedSize = this->recordSizeBytes * numRecordsInFile + HEADER_SIZE;

		if( expectedSize != fileSize )
		{
			printf( "Requested lineardb3 file %s does not contain a "
					   "whole number of %d-byte records.  "
					   "Assuming final record is garbage and truncating it.\n",
					   this->filename.c_str(), this->recordSizeBytes );
		   char tempPath[200];
		   sprintf( tempPath, "%.190s%s", this->filename.c_str(), ".trunc" );
		   FILE *tempFile = fopen( tempPath, "wb" );
		   if( tempFile == NULL ) throw exception("Failed to open temp file %s for truncation"/*, tempPath*/);

		   if( fseeko( this->file, 0, SEEK_SET ) ) throw exception("File seek operation failed");

		   unsigned char headerBuffer[HEADER_SIZE];

		   int numRead = fread( headerBuffer, HEADER_SIZE, 1, this->file );
		   if( numRead != 1 )throw exception("Failed to read header from lineardb3 file %s"/*, this->filename.c_str()*/);

		   int numWritten = fwrite( headerBuffer, HEADER_SIZE, 1, tempFile );
		   if( numWritten != 1 ) throw exception ("Failed to write header to temp lineardb3 truncation file %s"/*, tempPath*/);

		   for( uint64_t i=0; i<numRecordsInFile; i++ )
		   {
				numRead = fread( this->recordBuffer, this->recordSizeBytes, 1, this->file );
				if( numRead != 1 ) throw exception("Failed to read record from lineardb3 file %s"/*, this->filename.c_str()*/);

				numWritten = fwrite( this->recordBuffer, this->recordSizeBytes, 1, tempFile );
				if( numWritten != 1 ) throw exception("Failed to record to temp lineardb3 truncation file %s"/*, tempPath*/);
		   }
		   fclose( this->file );
		   fclose( tempFile );
		   if( rename( tempPath, this->filename.c_str() ) != 0 )throw exception("Failed overwrite lineardb3 file %s with truncation file %s"/*, this->filename.c_str(), tempPath*/);
		   this->file = fopen( this->filename.c_str(), "r+b" );
		   if( this->file == NULL ) throw exception("Failed to re-open lineardb3 file %s after trunctation"/*, this->filename.c_str()*/);
		}

		// now populate hash table
		uint32_t minTableBuckets = this->getComputedTableSize(numRecordsInFile);
		this->hashTableSizeA = minTableBuckets;
		this->hashTableSizeB = minTableBuckets;
		this->recomputeFingerprintMod();
		this->hashTable->init(this->hashTableSizeA);
		this->overflowBuckets->init(2);

		if(fseeko(this->file, HEADER_SIZE, SEEK_SET)) throw new openLife::system::object::entity::Exception("linearDB file operation failed");

		for( uint64_t i=0; i<numRecordsInFile; i++ )
		{
			if(fread(this->recordBuffer, this->recordSizeBytes, 1, this->file) != 1) throw new openLife::system::object::entity::Exception("Failed to read record from linearDB file");

			// put only in RAM part of table
			// note that this assumes that each key in the file is unique
			// (it should be, because we generated the file on a previous run)
			int result = 0;//TODO: a virer
			/*
			int result = LINEARDB3_getOrPut( inDB,
										   &( inDB->recordBuffer[0] ),
										   &( inDB->recordBuffer[inDB->keySize] ),
										   true,
										   // ignore data file
										   // update ram only
										   // don't even verify keys in data file
										   // this preserves our fread position
										   true );//mode_put && ignoreDataFile
										   */

			if( result != 0 ) throw new openLife::system::object::entity::Exception("Putting lineardb record in RAM hash table failed");
		}
		   this->lastOp = opRead;
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
	/* already done
	unsigned char key[8];
	unsigned char value[12];
	int *outSecondPlaceBiome;
	double *outSecondPlaceGap;
	intPairToKey( inX, inY, key );// look for changes to default in database
	*/
	memcpy(this->currentRecord.key, key, sizeof(this->currentRecord.key));
	std::cout << "\nSizeof key :" << sizeof(this->currentRecord.key);
	int afterPosX = valueToInt( this->currentRecord.key );
	int afterPosY = valueToInt( &( this->currentRecord.key[4] ) );
	std::cout << "\nRecherche du biome ("<<afterPosX<<","<<afterPosY<<")";

	//int result = LINEARDB3_get( &biomeDB, key, value );//TODO: search LINEARDB3_get ans replace with newBiomeDB->get(...)
	//int result = this->get1();

	int biome = -1;
	/*
	if( result == 0 )
	{
		// found
		int biome = valueToInt( &( value[0] ) );

		if( outSecondPlaceBiome != NULL ) {
			*outSecondPlaceBiome = valueToInt( &( value[4] ) );
		}

		if( outSecondPlaceGap != NULL ) {
			*outSecondPlaceGap = valueToInt( &( value[8] ) ) / gapIntScale;
		}
	}
	*/

	/*
	 => int biomeDBGet( int inX, int inY, int *outSecondPlaceBiome, double *outSecondPlaceGap)
	 	=> int LINEARDB3_get( LINEARDB3 *inDB, const void *inKey, void *outValue );
	 		=> int LINEARDB3_getOrPut( LINEARDB3 *inDB, const void *inKey, void *inOutValue, char inPut, char inIgnoreDataFile = false );
	 			X=> uint64_t getBinNumber( LINEARDB3 *inDB, uint32_t inFingerprint );
	 			=> uint64_t getBinNumber( LINEARDB3 *inDB, const void *inKey, uint32_t *outFingerprint );*
	 				=> uint64_t getBinNumberFromHash( LINEARDB3 *inDB, uint64_t inHashVal );
	 */
	return biome;
}

int openLife::system::object::store::device::random::LinearDB::get1()
{

	uint32_t fingerprint;
	//uint64_t binNumber = getBinNumber( inDB, inKey, &fingerprint );
	uint64_t binNumber = this->getBinNumber(&fingerprint);
	std::cout << "\nbin number : " << binNumber;

	/*
	unsigned int overflowDepth = 0;
	LINEARDB3_FingerprintBucket *thisBucket = getBucket( inDB->hashTable, binNumber );
	char skipToOverflow = false;
	if( inPut && inIgnoreDataFile ) {
		skipToOverflow = true;
	}
	if( !skipToOverflow || thisBucket->overflowIndex == 0 )
		for( int i=0; i<RECORDS_PER_BUCKET; i++ ) {
			int result = LINEARDB3_considerFingerprintBucket(
					inDB, inKey, inOutValue,
					fingerprint,
					inPut, inIgnoreDataFile,
					thisBucket,
					i );
			if( result < 2 ) {
				return result;
			}
			// 2 means record didn't match, keep going
		}

	uint32_t thisBucketIndex = 0;
		while( thisBucket->overflowIndex > 0 ) {
			// consider overflow
			overflowDepth++;
			if( overflowDepth > inDB->maxOverflowDepth ) {
				inDB->maxOverflowDepth = overflowDepth;
			}
			thisBucketIndex = thisBucket->overflowIndex;
			thisBucket = getBucket( inDB->overflowBuckets, thisBucketIndex );
			if( !skipToOverflow || thisBucket->overflowIndex == 0 )
				for( int i=0; i<RECORDS_PER_BUCKET; i++ ) {

					int result = LINEARDB3_considerFingerprintBucket(
							inDB, inKey, inOutValue,
							fingerprint,
							inPut, inIgnoreDataFile,
							thisBucket,
							i );

					if( result < 2 ) {
						return result;
					}
					// 2 means record didn't match, keep going
				}
		}
		if( inPut &&
		thisBucket->overflowIndex == 0 ) {
			// reached end of overflow chain without finding place to put value
			// need to make a new overflow bucket
			overflowDepth++;
			if( overflowDepth > inDB->maxOverflowDepth ) {
				inDB->maxOverflowDepth = overflowDepth;
			}
			thisBucket->overflowIndex =
					getFirstEmptyBucketIndex( inDB->overflowBuckets );
			FingerprintBucket *newBucket = getBucket( inDB->overflowBuckets,
													  thisBucket->overflowIndex );
			newBucket->fingerprints[0] = fingerprint;
			// will go at end of file
			newBucket->fileIndex[0] = inDB->numRecords;
			inDB->numRecords++;
			if( ! inIgnoreDataFile ) {
				uint64_t filePosRec =
						LINEARDB3_HEADER_SIZE +
						newBucket->fileIndex[0] *
						inDB->recordSizeBytes;
				// don't seek unless we have to
				if( inDB->lastOp == opRead ||
				ftello( inDB->file ) != (signed)filePosRec ) {
					// go to end of file
					if( fseeko( inDB->file, 0, SEEK_END ) ) {
						return -1;
					}
					// make sure it matches where we've documented that
					// the record should go
					if( ftello( inDB->file ) != (signed)filePosRec ) {
						return -1;
					}
				}
				int numWritten = fwrite( inKey, inDB->keySize, 1, inDB->file );
				inDB->lastOp = opWrite;
				numWritten += fwrite( inOutValue, inDB->valueSize, 1, inDB->file );
				if( numWritten != 2 ) {
					return -1;
				}
				return 0;
			}
			return 0;
		}
		// not found
		return 1;
	 */
	return 0;
}

//return LINEARDB3_getOrPut( inDB, inKey, outValue, false, false );

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
 * private
 **/

/**
 *
 */
void openLife::system::object::store::device::random::LinearDB::createResource()
{
	this->file = fopen(this->filename.c_str(), "w+b");
}

/**
 *
 * @return bool
 * @note return false on fail
 */
int openLife::system::object::store::device::random::LinearDB::createResourceHeader()
{
	// returns 0 on success, -1 on error
	if(fseeko(this->file, 0, SEEK_SET)) return false;
	if(fwrite(this->magicString.c_str(), this->magicString.length(), 1, this->file) != 1) return false;
	if(fwrite(&(this->keySize), sizeof(uint32_t), 1, this->file ) != 1) return false;
	if(fwrite( &(this->valueSize), sizeof(uint32_t), 1, this->file) != 1) return false;
	return true;
}

/**
 *
 * @return
 * @note same as => uint64_t getBinNumber( LINEARDB3 *inDB, const void *inKey, uint32_t *outFingerprint );
 * => uint64_t getBinNumberFromHash( LINEARDB3 *inDB, uint64_t inHashVal );
 */
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

/**
 *
 * @param inNumRecords
 * @return
 */
uint32_t openLife::system::object::store::device::random::LinearDB::getComputedTableSize(uint32_t inNumRecords)
{
	uint32_t minTableRecords = (uint32_t)ceil( inNumRecords / this->maxLoad );

	uint32_t minTableBuckets =(uint32_t)ceil( (double)minTableRecords /
			(double)RECORDS_PER_BUCKET );

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
	this->fingerprintMod = this->hashTableSizeA;

	while( true )
	{
		uint32_t newMod = this->fingerprintMod * 2;
		if( newMod <= this->fingerprintMod ) {
			// reached 32-bit limit
			return;
		}
		else {
			this->fingerprintMod = newMod;
		}
	}
}