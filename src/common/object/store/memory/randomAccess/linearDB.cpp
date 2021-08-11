//
// Created by olivier on 09/08/2021.
//

#include "linearDB.h"

#include <iostream>

#include "OneLife/server/lineardb3.h"

extern char anyBiomesInDB;
extern int maxBiomeXLoc;
extern int maxBiomeYLoc;
extern int minBiomeXLoc;
extern int minBiomeYLoc;

double gapIntScale = 1000000.0;

common::object::store::memory::randomAccess::LinearDB::LinearDB(LINEARDB3* dbState)
{
	this->dbState = dbState;
}

common::object::store::memory::randomAccess::LinearDB::~LinearDB() {}

int common::object::store::memory::randomAccess::LinearDB::get(unsigned char idx[8], unsigned char value[12])
{
	return LINEARDB3_get(this->dbState, idx, value);
}

void common::object::store::memory::randomAccess::LinearDB::put(int idx, int value) {}

