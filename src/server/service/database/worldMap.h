//
// Created by olivier on 07/08/2021.
//

#ifndef OPENLIFE_SERVER_SERVICE_DATABASE_WORLDMAP_H
#define OPENLIFE_SERVER_SERVICE_DATABASE_WORLDMAP_H

#include "src/system/_base/object/abstract/service.h"

#include <string>
#include <vector>
#include <array>
#include "src/server/type/settings.h"
#include "src/server/type/entities.h"
#include "src/system/_base/type/entities.h"
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
			void updateSecondPlaceIndex(int *outSecondPlaceIndex);
			void updateSecondPlaceGap(double *outSecondPlaceGap);
			int init();
			void setMapSeed(unsigned int x, unsigned int y);

			//!data management methods
			WorldMap* select(int posX, int posY);
			WorldMap* create();
			void insert(openLife::system::type::entity::Biome biome);
			void insert(common::object::entity::MapZone* mapZone);
			openLife::system::type::entity::Biome getNewBiome();
			int getBiome();
			openLife::system::type::entity::Biome getBiomeRecord(char forceValue = true);

			//!data information methods
			std::vector<int> getBiomes();//TODO should return array of biome struct
			int getInfoBiome(int biome);//TODO should return biome struct

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
				std::vector<unsigned int> relief;
				std::vector<int> specialBiomeBandYCenter;
				std::vector<int> specialBiomeBandOrder;
				std::vector<int> specialBiomes;
				std::vector<float> biomeWeight;
			}map;

			std::vector<int> biome;
			std::vector<int> mappedBiomeValue;

			openLife::system::object::store::memory::ExtendedVector2D<openLife::system::type::entity::Biome>* dbBiomeCache;
			openLife::system::object::store::memory::random::Biome* dbCacheBiome;
			openLife::system::object::store::device::random::LinearDB* dbBiome;

			struct
			{
				std::vector<int> order;
			}dataBiome;

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
