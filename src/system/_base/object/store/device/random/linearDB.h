//
// Created by olivier on 09/08/2021.
//

#ifndef OPENLIFE_COMMON_OBJECT_STORE_MEMORY_RANDOMACCESS_LINEARDB_H
#define OPENLIFE_COMMON_OBJECT_STORE_MEMORY_RANDOMACCESS_LINEARDB_H

// some compilers require this to access UINT64_MAX
#define __STDC_LIMIT_MACROS

#include <stdint.h>
#include <stdio.h>

#include "src/system/_base/_macro/environment.h"

#include "src/system/_base/settings/linearDB.h"
#include "src/common/type/database/lineardb3.h"
#include "minorGems/io/file/File.h"
#include "minorGems/util/log/AppLog.h"
//#include "src/common/process/hash.h"

uint64_t MurmurHash64B ( const void * key, int len, uint64_t seed );

#define MurmurHash64 MurmurHash64B
#define LINEARDB3_hash(inB, inLen) MurmurHash64( inB, inLen, 0xb9115a39 )




#define FingerprintBucket LINEARDB3_FingerprintBucket
#define RECORDS_PER_BUCKET LINEARDB3_RECORDS_PER_BUCKET
#define PageManager LINEARDB3_PageManager
#define LINEARDB3_HEADER_SIZE 11
#define BUCKETS_PER_PAGE LINEARDB3_BUCKETS_PER_PAGE
#define BucketPage LINEARDB3_BucketPage

namespace openLife::system::object::store::device::random
{
	class LinearDB
	{
		public:
			LinearDB(openLife::system::settings::LinearDB settings);
			~LinearDB();

			void put(int idx, int value);
			int get(void* key);

			int isResourceExist();

			void init(LINEARDB3 *dbState);

			static const double MAX_LOAD_FOR_OPEN_CALLS;

		private:
			void createResource();

			std::string filename;
			unsigned int keySize;
			unsigned int valueSize;



			FILE *file;
			LINEARDB3* dbState;
			double maxLoad;// load above this causes table to expand incrementally
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
	};
}

#if !defined(OPENLIFE_UNIT_TEST)
	#include "draft.h"
#endif

#endif //OPENLIFE_COMMON_OBJECT_STORE_MEMORY_RANDOMACCESS_LINEARDB_H
