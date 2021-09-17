//
// Created by olivier on 15/09/2021.
//

#ifndef OPENLIFE_SERVER_TYPE_SETTINGS_H
#define OPENLIFE_SERVER_TYPE_SETTINGS_H

#include <vector>
#include <string>
#include "src/system/_base/type.h"
#include "src/server/type/entities.h"

namespace openLife::server::settings
{
	typedef struct{
		struct{
			int type;
			struct{
				std::string filename;
			}sketch;
		}mapGenerator;
		std::string filename;
		openLife::system::type::Dimension2D mapSize;
		int specialBiomeBandMode;
		struct{
			openLife::system::type::Value2D_U32 seed;
			std::vector<int> specialBiomeBandYCenter;
			int specialBiomeBandThickness;
			std::vector<int> specialBiomeBandOrder;
			std::vector<int> specialBiomes;
			std::vector<float> biomeWeight;
		}map;
		struct{
			std::vector<int> order;
		}biome;
		std::vector<openLife::server::type::entity::Climate> climate;
	}WorldMap;
}

#endif //OPENLIFE_SERVER_TYPE_SETTINGS_H
