//
// Created by olivier on 07/08/2021.
//

#ifndef OPENLIFE_SERVER_SERVICE_DATABASE_WORLDMAP_H
#define OPENLIFE_SERVER_SERVICE_DATABASE_WORLDMAP_H

#include <string>
#include <vector>
#include <array>
#include "src/server/type/settings.h"
#include "src/server/type/entities.h"
#include "src/system/_base/type/record.h"
#include "src/system/_base/object/store/device/random/linearDB.h"
#include "src/system/_base/object/store/memory/random/biome.h"
#include "src/system/_base/object/store/memory/extendedVector2D.h"
#include "src/system/_base/type.h"
#include "src/common/type/database/lineardb3.h"
#include "src/common/object/entity/mapZone.h"

//!legacy
#include "OneLife/server/map.h"

namespace openLife::server::service::database::worldMap
{
	typedef struct
	{
		std::string label;
	}Biome;
}

namespace openLife::server::service::database
{
	class WorldMap
	{
		public:
			WorldMap(openLife::server::settings::WorldMap settings);
			~WorldMap();

			//!temporary methods
			void legacy(LINEARDB3* biomeDB, char* notEmptyDB, openLife::system::object::store::memory::random::Biome* dbCacheBiome);
			char* notEmptyDB;

			int init();
			void setMapSeed(unsigned int x, unsigned int y);

			WorldMap* select(int posX, int posY);
			WorldMap* create();
			void insert(openLife::system::type::record::Biome biome);
			void insert(common::object::entity::MapZone* mapZone);
			openLife::system::type::record::Biome getNewBiome();
			int getBiome();
			openLife::system::type::record::Biome getBiomeRecord(char forceValue = true);

			void updateSecondPlaceIndex(int *outSecondPlaceIndex);
			void updateSecondPlaceGap(double *outSecondPlaceGap);

		private:
			unsigned int width;
			unsigned int height;
			unsigned int detail;
			unsigned int idxMax;
			struct {int x; int y;} query;
			struct {unsigned int x; unsigned int y;} center;
			std::vector<int> mapTile;

			struct{
				int type;
				struct{
					std::string filename;
				}sketch;
			}mapGenerator;

			struct{
				int specialBiomeBandMode;
				openLife::system::type::Value2D_U32 seed;
				char allowSecondPlaceBiomes;
				int specialBiomeBandThickness;
				std::vector<int> specialBiomeBandYCenter;
				std::vector<int> specialBiomeBandOrder;
				std::vector<int> specialBiomes;
				std::vector<float> biomeWeight;
			}map;

			openLife::system::object::store::memory::ExtendedVector2D<openLife::system::type::record::Biome>* dbBiomeCache;
			openLife::system::object::store::memory::random::Biome* dbCacheBiome;
			openLife::system::object::store::device::random::LinearDB* dbBiome;

			struct
			{
				std::vector<int> order;
			}dataBiome;
			std::vector<openLife::server::service::database::worldMap::Biome> biome;

			struct mapSettings{
				float *biomeCumuWeights;
			};

			LINEARDB3* biomeDB;

			struct{
				int *outSecondPlaceIndex;
				double *outSecondPlaceGap;
			}tmp;
	};
}

#endif //OPENLIFE_SERVER_SERVICE_DATABASE_WORLDMAP_H
