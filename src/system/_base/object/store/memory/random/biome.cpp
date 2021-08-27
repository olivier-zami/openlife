//
// Created by olivier on 26/08/2021.
//

#include "biome.h"

#include "src/server/main.h"
#include <iostream>

openLife::system::object::store::memory::random::Biome::Biome(unsigned int size)
{
	this->biome.reserve(size);
	openLife::system::type::record::Biome undefinedBiome = {0, 0, -1, 0, 0};
	for(unsigned int i=0; i<this->biome.capacity(); i++) this->biome[i] = undefinedBiome;
	std::cout << "\nOpen biomeCached for size : " << this->biome.capacity();
}

openLife::system::object::store::memory::random::Biome::~Biome()
{
	//std::cout << "\n############### Fermeture de l'objet cachedBiome ...";
}

void openLife::system::object::store::memory::random::Biome::put(int idx, openLife::system::type::record::Biome record)
{
	std::cout << "\nTrying to save record["<<idx<<"] {"<<record.x<<", "<<record.y<<", "<<record.value<<", "<<record.secondPlace<<", "<<record.secondPlaceGap<<"} => key ";
	this->biome[idx] = record;
}

void openLife::system::object::store::memory::random::Biome::put(openLife::system::type::record::Biome record)
{
	std::cout << "\nTrying to save record {"<<record.x<<", "<<record.y<<", "<<record.value<<", "<<record.secondPlace<<", "<<record.secondPlaceGap<<"} => key " << this->generateHashKey(record.x, record.y);
	this->put(this->generateHashKey(record.x, record.y), record);
}

openLife::system::type::record::Biome openLife::system::object::store::memory::random::Biome::get(int idx)
{
	return this->biome[idx];
}

openLife::system::type::record::Biome openLife::system::object::store::memory::random::Biome::get(int x, int y)
{
	int idx = this->generateHashKey(x, y);
	openLife::system::type::record::Biome biomeRecord = this->biome[idx];
	std::cout << "\nSeek for biome ("<<x<<", "<<y<<") Capacity : " << this->biome.capacity() << " << index : " << idx << " => {" <<biomeRecord.x<<", "<<biomeRecord.y<<", "<<biomeRecord.value<<", "<<biomeRecord.secondPlace<<", "<<biomeRecord.secondPlaceGap<< "}";
	return biomeRecord;
}


int openLife::system::object::store::memory::random::Biome::generateHashKey(int x, int y)
{
	int hashKey = ( x * CACHE_PRIME_A + y * CACHE_PRIME_B ) % BIOME_CACHE_SIZE;
	if( hashKey < 0 ) hashKey += BIOME_CACHE_SIZE;
	return hashKey;
}