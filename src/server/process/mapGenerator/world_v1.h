//
// Created by olivier on 01/09/2021.
//

#ifndef OPENLIFE_WORLD_V1_H
#define OPENLIFE_WORLD_V1_H

#include "src/system/_base/type.h"
#include "src/system/_base/type/entities.h"

namespace openLife::server::procedure::mapGenerator
{
	class World_v1
	{
		public:
			World_v1(
					openLife::system::type::Size2D_32 mapSize,
					openLife::system::type::Value2D_U32 mapCenter);

			~World_v1();

			openLife::system::type::entity::Biome getBiome(int x, int y);

			openLife::system::type::Value2D_U32 getMapCenter();

		private:
			struct{
				openLife::system::type::Size2D_32 size;
				openLife::system::type::Value2D_U32 center;
			}map;
	};
}

#endif //OPENLIFE_WORLD_V1_H
