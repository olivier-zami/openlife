//
// Created by olivier on 11/08/2021.
//

#ifndef ONELIFETEST_LINEARDB3_H
#define ONELIFETEST_LINEARDB3_H

// larger values here reduce RAM overhead per record slightly
// and may speed up lookup in over-full tables, but might slow
// down lookup in less full tables.
#define LINEARDB3_RECORDS_PER_BUCKET 8
#define LINEARDB3_BUCKETS_PER_PAGE 4096

#include <cstdint>
#include <cstdio>

enum LastFileOp{ opRead, opWrite };

typedef struct
{
	// load above this causes table to expand incrementally
	double maxLoad;
	// number of inserted records in database
	uint32_t numRecords;
	// for linear hashing table expansion
	// number of slots in base table
	uint32_t hashTableSizeA;
	// number of slots in expanded table
	// when this reaches hashTableSizeA * 2
	// hash table is done with a full round of expansion
	// and hashTableSizeA is set to hashTableSizeB at that point
	uint32_t hashTableSizeB;
	unsigned int keySize;
	unsigned int valueSize;
	FILE *file;
	// for deciding when fseek is needed between reads and writes
	LastFileOp lastOp;
	// equal to the largest possible 32-bit table size, given
	// our current table size
	// used as mod for computing 32-bit hash fingerprints
	uint32_t fingerprintMod;
	unsigned int recordSizeBytes;
	uint8_t *recordBuffer;
	unsigned int maxOverflowDepth;
	// sized to hashTableSizeB buckets
	struct PageManagerState *hashTable;
	struct PageManagerState *overflowBuckets;
}LINEARDB3; //LinearBDState

typedef struct PageManagerState{
	uint32_t numBuckets;
	// number of allocated pages
	uint32_t numPages;
	// number of slots in pages pointer array
	// beyond numPages, there are NULL pointers
	uint32_t pageAreaSize;
	struct BucketPageState **pages;
	uint32_t firstEmptyBucket;
} LINEARDB3_PageManager;

typedef struct BucketFingerPrint{
	// index of another FingerprintBucket in the overflow array,
	// or 0 if no overflow (0 overflow bucket never used)
	uint32_t overflowIndex;

	// record number in the data file
	uint32_t fileIndex[ LINEARDB3_RECORDS_PER_BUCKET ];

	// fingerprint mini-hash, mod the largest possible table size
	// in the 32-bit space.  We can mod this with our current table
	// size to find the current bin number (rehashing without
	// actually rehashing the full key)
	uint32_t fingerprints[ LINEARDB3_RECORDS_PER_BUCKET ];
} LINEARDB3_FingerprintBucket;

typedef struct BucketPageState{
	struct BucketFingerPrint buckets[ LINEARDB3_BUCKETS_PER_PAGE ];
} LINEARDB3_BucketPage;



#endif //ONELIFETEST_LINEARDB3_H
