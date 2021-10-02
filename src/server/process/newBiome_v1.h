//
// Created by olivier on 29/08/2021.
//

#ifndef OPENLIFE_NEWBIOME_H
#define OPENLIFE_NEWBIOME_H

#include <vector>
#include "src/system/_base/type.h"
#include "src/system/_base/type/entities.h"

namespace openLife::server::process
{
	::openLife::system::type::entity::Biome newBiome_v1(
		int x, int y,
		openLife::system::type::Value2D_U32 randSeed,
		char allowSecondPlaceBiomes,
		std::vector<int> biomes,
		int specialBiomeBandThickness,
		std::vector<int> specialBiomeBandYCenter,
		std::vector<int> specialBiomeBandIndexOrder,
		int specialBiomeBandMode,
		std::vector<int> specialBiomes,
		std::vector<float> biomeWeights);
}
#endif //OPENLIFE_NEWBIOME_H
