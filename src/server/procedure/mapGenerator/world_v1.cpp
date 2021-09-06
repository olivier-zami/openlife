//
// Created by olivier on 01/09/2021.
//

#include "world_v1.h"

openLife::server::procedure::mapGenerator::World_v1::World_v1(
		openLife::system::type::Size2D_32 mapSize,
		openLife::system::type::Value2D_U32 mapCenter)
{
	this->map.size = mapSize;
	this->map.center = mapCenter;
}

openLife::server::procedure::mapGenerator::World_v1::~World_v1() {}

openLife::system::type::record::Biome openLife::server::procedure::mapGenerator::World_v1::getBiome(int x, int y)
{
	openLife::system::type::record::Biome biome = {x, y, -1, 0, 0};
	return biome;
}

openLife::system::type::Value2D_U32 openLife::server::procedure::mapGenerator::World_v1::getMapCenter()
{
	return this->map.center;
}