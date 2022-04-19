//
// Created by olivier on 19/08/2021.
//

#include "pageManager.h"

#include <cstring>

openLife::system::object::store::device::random::linearDB::PageManager::PageManager() {}
openLife::system::object::store::device::random::linearDB::PageManager::~PageManager() {}

/**
 *
 * @param inNumStartingBuckets
 */
void openLife::system::object::store::device::random::linearDB::PageManager::init(uint32_t inNumStartingBuckets)
{
	this->firstEmptyBucket = 0;
	this->numBuckets = inNumStartingBuckets;
	this->numPages = 1 + inNumStartingBuckets / BUCKETS_PER_PAGE;
	this->pageAreaSize = 2 * this->numPages;
	this->pages = new Page*[this->pageAreaSize]; //new BucketPage

	for(uint32_t i=0; i<this->pageAreaSize; i++) this->pages[i] = NULL;
	for(uint32_t i=0; i<this->numPages; i++)
	{
		this->pages[i] = new openLife::system::object::store::device::random::linearDB::Page;
		memset(this->pages[i], 0, sizeof(openLife::system::object::store::device::random::linearDB::Page));
	}
}