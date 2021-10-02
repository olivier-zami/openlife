//
// Created by olivier on 26/08/2021.
//

#include "biome.h"

#include "src/server/main.h"

openLife::system::object::store::memory::random::Biome::Biome(unsigned int size)
{
	this->biome.reserve(size);
	openLife::system::type::entity::Biome undefinedBiome = {0, 0, -1, 0, 0};
	for(unsigned int i=0; i<this->biome.capacity(); i++) this->biome[i] = undefinedBiome;
}

openLife::system::object::store::memory::random::Biome::~Biome()
{
	//std::cout << "\n############### Fermeture de l'objet cachedBiome ...";
}

void openLife::system::object::store::memory::random::Biome::put(int idx, openLife::system::type::entity::Biome record)
{
	this->biome[idx] = record;
}

void openLife::system::object::store::memory::random::Biome::put(openLife::system::type::entity::Biome record)
{
	this->put(this->generateHashKey(record.x, record.y), record);
}

openLife::system::type::entity::Biome openLife::system::object::store::memory::random::Biome::get(int idx)
{
	return this->biome[idx];
}

openLife::system::type::entity::Biome openLife::system::object::store::memory::random::Biome::get(int x, int y)
{
	int idx = this->generateHashKey(x, y);
	openLife::system::type::entity::Biome biomeRecord = this->biome[idx];
	return biomeRecord;
}


int openLife::system::object::store::memory::random::Biome::generateHashKey(int x, int y)
{
	int hashKey = ( x * CACHE_PRIME_A + y * CACHE_PRIME_B ) % BIOME_CACHE_SIZE;
	if( hashKey < 0 ) hashKey += BIOME_CACHE_SIZE;
	return hashKey;
}