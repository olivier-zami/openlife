//
// Created by olivier on 19/08/2021.
//

#ifndef OPENLIFE_PAGEMANAGER_H
#define OPENLIFE_PAGEMANAGER_H

#include <stdint.h>

#define BUCKETS_PER_PAGE 4096
#define RECORDS_PER_BUCKET 8

namespace openLife::system::object::store::device::random::linearDB
{
	typedef struct
	{
		// index of another FingerprintBucket in the overflow array,
		// or 0 if no overflow (0 overflow bucket never used)
		uint32_t overflowIndex;

		// record number in the data file
		uint32_t fileIndex[RECORDS_PER_BUCKET];

		// fingerprint mini-hash, mod the largest possible table size
		// in the 32-bit space.  We can mod this with our current table
		// size to find the current bin number (rehashing without
		// actually rehashing the full key)
		uint32_t fingerprints[RECORDS_PER_BUCKET];
	}Bucket;

	typedef struct
	{
		Bucket buckets[BUCKETS_PER_PAGE];
	}Page;

	class PageManager
	{
		public:
			PageManager();
			~PageManager();
			void init(uint32_t inNumStartingBuckets);
		private:
			uint32_t numBuckets;
			// number of allocated pages
			uint32_t numPages;
			// number of slots in pages pointer array
			// beyond numPages, there are NULL pointers
			uint32_t pageAreaSize;
			uint32_t firstEmptyBucket;
			Page **pages;
	};
};

#endif //OPENLIFE_PAGEMANAGER_H
