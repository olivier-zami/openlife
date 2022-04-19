//
// Created by olivier on 09/08/2021.
//

#ifndef OPENLIFE_COMMON_OBJECT_STORE_MEMORY_RANDOMACCESS_LINEARDB_H
#define OPENLIFE_COMMON_OBJECT_STORE_MEMORY_RANDOMACCESS_LINEARDB_H

// some compilers require this to access UINT64_MAX
#define __STDC_LIMIT_MACROS

//#include "src/system/_base/_macro/environment.h"
#include "src/system/_base/object/store/device/random/linearDB/pageManager.h"

#include <stdint.h>
#include <stdio.h>
#include <string>

//<<<<<<< Updated upstream:src/common/object/store/memory/randomAccess/linearDB.h

//=======
//#include "src/system/_base/settings/linearDB.h"
//>>>>>>> Stashed changes:src/system/_base/object/store/device/random/linearDB.h

#include "src/common/type/database/lineardb3.h"
#include "minorGems/io/file/File.h"
#include "src/third_party/jason_rohrer/minorGems/util/log/AppLog.h"
//#include "src/common/process/hash.h"

uint64_t MurmurHash64B ( const void * key, int len, uint64_t seed );

#define MurmurHash64 MurmurHash64B
#define LINEARDB3_hash(inB, inLen) MurmurHash64( inB, inLen, 0xb9115a39 )

#define FingerprintBucket LINEARDB3_FingerprintBucket
#define LINEARDB3_HEADER_SIZE 11
#define BucketPage LINEARDB3_BucketPage

#define KISSDB_OPEN_MODE_RDONLY 1		//Open mode: read only
#define KISSDB_OPEN_MODE_RDWR 2			//Open mode: read/write
#define KISSDB_OPEN_MODE_RWCREAT 3		//Open mode: read/write, create if doesn't exist
#define KISSDB_OPEN_MODE_RWREPLACE 4	//Open mode: truncate database, open for reading and writing

namespace openLife::system::settings
{
	typedef struct{
		std::string filename;
		int mode;
		unsigned int hTableSize;
		struct{
			unsigned int keySize;
			unsigned int valSize;
		}record;
	}LinearDB;
}

namespace openLife::system::object::store::device::random
{
	class LinearDB
	{
		public:
			LinearDB(openLife::system::settings::LinearDB settings);
			~LinearDB();

			void handle(LINEARDB3* db);

			void set(int key, int value);
			int get(void* key);

			int isResourceExist();

			static const double MAX_LOAD_FOR_OPEN_CALLS;

			//TODO: change to private
			LINEARDB3* db;
			::openLife::system::settings::LinearDB settings;

		private:
			void createResource();
			//TODO: to restore
			//int createResourceHeader();
			uint32_t getComputedTableSize(uint32_t inNumRecords);
			//uint64_t getBinNumber(uint32_t *outFingerprint);
			void recomputeFingerprintMod();
			//int get1();

			std::string filename;
			unsigned int keySize;
			unsigned int valueSize;

			FILE *file;


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
			/*TODO: to restore
			openLife::system::object::store::device::random::linearDB::PageManager* hashTable;
			openLife::system::object::store::device::random::linearDB::PageManager* overflowBuckets;
			*/
			struct PageManagerState *hashTable;
			struct PageManagerState *overflowBuckets;
	};
}

/**********************************************************************************************************************/

void dbLookTimePut( int inX, int inY, timeSec_t inTime );
timeSec_t dbLookTimeGet( int inX, int inY );

#if !defined(OPENLIFE_UNIT_TEST)
	#include "draft.h"
#endif

#endif //OPENLIFE_COMMON_OBJECT_STORE_MEMORY_RANDOMACCESS_LINEARDB_H
