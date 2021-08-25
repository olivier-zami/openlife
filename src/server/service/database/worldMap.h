//
// Created by olivier on 07/08/2021.
//

#ifndef OPENLIFE_SERVER_SERVICE_DATABASE_WORLDMAP_H
#define OPENLIFE_SERVER_SERVICE_DATABASE_WORLDMAP_H

#include <vector>
#include <array>
#include "src/system/_base/settings/database/worldMap.h"
#include "src/system/_base/type/record.h"
#include "src/system/_base/object/store/device/random/linearDB.h"
#include "src/common/type/database/lineardb3.h"
#include "src/common/object/entity/mapZone.h"

namespace openLife::server::service::database
{
	class WorldMap
	{
		public:
			WorldMap(openLife::system::settings::database::WorldMap settings/*unsigned int width, unsigned int height, unsigned int detail*/);
			~WorldMap();

			//!temporary methods
			void legacy(LINEARDB3* biomeDB, char* notEmptyDB);
			openLife::system::settings::database::WorldMap settings;
			char* notEmptyDB;

			int init();
			WorldMap* select(int posX, int posY);
			void insert(openLife::system::type::record::Biome biome);
			void insert(common::object::entity::MapZone* mapZone);
			int getBiome();
			openLife::system::type::record::Biome getBiomeRecord();

			void useBiomeStorehouse(openLife::system::object::store::device::random::LinearDB* biomeStoreHouse);

			void updateSecondPlaceIndex(int *outSecondPlaceIndex);
			void updateSecondPlaceGap(double *outSecondPlaceGap);

		private:
			unsigned int width;
			unsigned int height;
			unsigned int detail;
			unsigned int idxMax;
			struct {int x; int y;} query;
			struct {unsigned int x; unsigned int y;} center;
			std::vector<int> biome;
			openLife::system::object::store::device::random::LinearDB* biomeStoreHouse;

			LINEARDB3* biomeDB;

			struct{
				int *outSecondPlaceIndex;
				double *outSecondPlaceGap;
			}tmp;
	};
}

int biomeDBGet( int inX, int inY,
				int *outSecondPlaceBiome = nullptr,
				double *outSecondPlaceGap = nullptr);

/*
int getMapBiomeIndex( int inX, int inY,
					  int *outSecondPlaceIndex = nullptr,
					  double *outSecondPlaceGap = nullptr);*/
#endif //OPENLIFE_SERVER_SERVICE_DATABASE_WORLDMAP_H
