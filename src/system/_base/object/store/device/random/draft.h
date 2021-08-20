//
// Created by olivier on 14/08/2021.
//

#ifndef ONELIFETEST_DRAFT_H
#define ONELIFETEST_DRAFT_H

#include "src/common/type/database/lineardb3.h"

int DB_open_timeShrunk(
		LINEARDB3 *db,
		const char *path,
		int mode,
		unsigned long hash_table_size,
		unsigned long key_size,
		unsigned long value_size);

/**
 * Set maximum table load for all subsequent callst to LINEARDB3_open.
 *
 * Defaults to 0.5.
 *
 * When this load is surpassed, hash table expansion occurs.
 *
 * Lower values waste more RAM on table space but result in slightly higher
 * performance.
 *
 *
 * Note that a given DB remembers what maxLoad was set when it was opened,
 * and ignores future calls to setMaxLoad.
 *
 * However, the max load is NOT written into the database file format.
 *
 * Thus, data can be loaded into a table with a different load by setting
 * a differen maxLoad before re-opening the file.
 */
void LINEARDB3_setMaxLoad( double inMaxLoad );

/**
 * Open database
 *
 * The three _size parameters must be specified if the database could
 * be created or re-created. Otherwise an error will occur. If the
 * database already exists, these parameters are ignored and are read
 * from the database. You can check the struture afterwords to see what
 * they were.
 *
 * @param db Database struct
 * @param path Path to data file.
 * @param inMode is ignored, and always opened in RW-create mode
 *   (left for compatibility with KISSDB api)
 * @param inHashTableStartSize Size of hash table in entries
 *   This is the starting size of the table, which will grow as the table
 *   becomes full.  If less than 2, will be automatically raised to 2.
 * @param key_size Size of keys in bytes
 * @param value_size Size of values in bytes
 * @return 0 on success, nonzero on error
 */
int LINEARDB3_open(
		LINEARDB3 *inDB,
		const char *inPath,
		int inMode,
		unsigned int inHashTableStartSize,
		unsigned int inKeySize,
		unsigned int inValueSize );

/**
 * Close database
 *
 * @param db Database struct
 */
void LINEARDB3_close( LINEARDB3 *inDB );

/**
 * Put an entry (overwriting it if it already exists)
 *
 * In the already-exists case the size of the database file does not
 * change.
 *
 * @param db Database struct
 * @param key Key (key_size bytes)
 * @param value Value (value_size bytes)
 * @return -1 on I/O error, 0 on success
 */
int LINEARDB3_put( LINEARDB3 *inDB, const void *inKey, const void *inValue );

/**
 * Cursor used for iterating over all entries in database
 */
typedef struct {
	LINEARDB3 *db;
	uint32_t nextRecordIndex;
} LINEARDB3_Iterator;

/**
 * Initialize an iterator
 *
 * @param db Database struct
 * @param i Iterator to initialize
 */
void LINEARDB3_Iterator_init( LINEARDB3 *inDB, LINEARDB3_Iterator *inDBi );



/**
 * Get the next entry
 *
 * The order of entries returned by iterator is undefined. It depends on
 * how keys hash.
 *
 * @param Database iterator
 * @param kbuf Buffer to fill with next key (key_size bytes)
 * @param vbuf Buffer to fill with next value (value_size bytes)
 * @return 0 if there are no more entries, negative on error,
 *         positive if an kbuf/vbuf have been filled
 */
int LINEARDB3_Iterator_next( LINEARDB3_Iterator *inDBi,
							 void *outKey, void *outValue );

/**
 * More advanced functions below.
 *
 * These can be ignored for most usages, which just need open, close,
 * get, put, and iterators.
 */





/**
 * Total number of cells in table, including those added through
 * incremental expansion due to Linear Hashing algorithm.
 */
unsigned int LINEARDB3_getCurrentSize( LINEARDB3 *inDB );


/**
 * Number of records that have been inserted in the database.
 */
unsigned int LINEARDB3_getNumRecords( LINEARDB3 *inDB );




/**
 * Gets optimal starting table size for a given load and number of records.
 *
 * Return value can be used for inHashTableStartSize in LINEARDB3_open.
 */
uint32_t LINEARDB3_getPerfectTableSize( double inMaxLoad,
										uint32_t inNumRecords );



/**
 * Gets the optimal starting table size, based on an existing inDB, to house
 * inNewNumRecords.  Pays attention to inDB's set maxLoad.
 * This is useful when iterating through one DB to insert items into a new,
 * smaller DB.
 *
 * Return value can be used for inHashTableStartSize in LINEARDB3_open.
 */
unsigned int LINEARDB3_getShrinkSize( LINEARDB3 *inDB,
									  unsigned int inNewNumRecords );

/**
 * Get an entry
 *
 * @param db Database struct
 * @param key Key (key_size bytes)
 * @param vbuf Value buffer (value_size bytes capacity)
 * @return -1 on I/O error, 0 on success, 1 on not found
 */
int LINEARDB3_get( LINEARDB3 *inDB, const void *inKey, void *outValue );

// if inIgnoreDataFile (which only applies if inPut is true), we completely
// ignore the data file and don't touch it, updating the RAM hash table only,
// and assuming all unique values on collision
int LINEARDB3_getOrPut( LINEARDB3 *inDB, const void *inKey, void *inOutValue,
						char inPut, char inIgnoreDataFile = false );

uint64_t getBinNumber( LINEARDB3 *inDB, const void *inKey,
							  uint32_t *outFingerprint );

// no bounds checking
LINEARDB3_FingerprintBucket *getBucket( LINEARDB3_PageManager *inPM,
									 uint32_t inBucketIndex );

int LINEARDB3_considerFingerprintBucket( LINEARDB3 *inDB,
									 const void *inKey,
									 void *inOutValue,
									 uint32_t inFingerprint,
									 char inPut,
									 char inIgnoreDataFile,
									 LINEARDB3_FingerprintBucket *inBucket,
									 int inRecIndex );

// always skips bucket at index 0
// assuming that this call is used for overflowBuckets only, where
// index 0 is used to mark buckets with no further overflow
uint32_t getFirstEmptyBucketIndex( LINEARDB3_PageManager *inPM );

uint64_t getBinNumberFromHash( LINEARDB3 *inDB, uint64_t inHashVal );

inline char keyComp( int inKeySize, const void *inKeyA, const void *inKeyB );

//  returns pointer to newly created bucket
LINEARDB3_FingerprintBucket *addBucket( LINEARDB3_PageManager *inPM );

int expandTable( LINEARDB3 *inDB );

typedef struct {
	LINEARDB3_FingerprintBucket *nextBucket;
	int nextRecord;
} BucketIterator;

void insertIntoBucket( LINEARDB3 *inDB,
					   BucketIterator *inBucketIterator,
					   uint32_t inFingerprint,
					   uint32_t inFileIndex );

void markBucketEmpty( LINEARDB3_PageManager *inPM, uint32_t inBucketIndex );

uint64_t getBinNumber( LINEARDB3 *inDB, uint32_t inFingerprint );

void initPageManager( LINEARDB3_PageManager *inPM, uint32_t inNumStartingBuckets );

void freePageManager( LINEARDB3_PageManager *inPM );

int getRecordSizeBytes( int inKeySize, int inValueSize );

void recomputeFingerprintMod( LINEARDB3 *inDB );

int writeHeader( LINEARDB3 *inDB );

#endif //ONELIFETEST_DRAFT_H