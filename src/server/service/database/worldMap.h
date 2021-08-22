//
// Created by olivier on 07/08/2021.
//

#ifndef OPENLIFE_WORLDMAP_H
#define OPENLIFE_WORLDMAP_H

#include <vector>
#include <array>
#include "src/system/_base/object/store/device/random/linearDB.h"
#include "src/common/object/entity/mapZone.h"

namespace openLife::server::service::database
{
	class WorldMap
	{
		public:
			WorldMap(unsigned int width, unsigned int height, unsigned int detail);
			~WorldMap();

			void insert(common::object::entity::MapZone* mapZone);
			WorldMap* select(int posX, int posY);
			int getBiome();

			void useBiomeStorehouse(openLife::system::object::store::device::random::LinearDB* biomeStoreHouse);

			void updateSecondPlaceIndex(int *outSecondPlaceIndex);
			void updateSecondPlaceGap(double *outSecondPlaceGap);

		private:
			unsigned int width;
			unsigned int height;
			unsigned int detail;
			unsigned int idxMax;
			struct {unsigned int x; unsigned int y;} query;
			struct {unsigned int x; unsigned int y;} center;
			std::vector<int> biome;
			openLife::system::object::store::device::random::LinearDB* biomeStoreHouse;

			struct{
				int *outSecondPlaceIndex;
				double *outSecondPlaceGap;
			}tmp;
	};
}

int biomeDBGet( int inX, int inY,
				int *outSecondPlaceBiome = nullptr,
				double *outSecondPlaceGap = nullptr);

int getMapBiomeIndex( int inX, int inY,
					  int *outSecondPlaceIndex = nullptr,
					  double *outSecondPlaceGap = nullptr);
#endif //OPENLIFE_WORLDMAP_H
