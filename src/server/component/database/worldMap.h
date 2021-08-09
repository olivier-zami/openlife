//
// Created by olivier on 07/08/2021.
//

#ifndef OPENLIFE_WORLDMAP_H
#define OPENLIFE_WORLDMAP_H

#include <vector>
#include <array>

typedef struct{
	unsigned int width;
	unsigned int height;
	std::array<int, 100> coord;
}MapZone;

namespace server::component::database
{
	class WorldMap
	{
		public:
			WorldMap(unsigned int width, unsigned int height, unsigned int detail);
			~WorldMap();

			void insert(MapZone mapZone);
			WorldMap* select(int posX, int posY);
			int getBiome();

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

			struct{
				int *outSecondPlaceIndex;
				double *outSecondPlaceGap;
			};
	};
}

int getMapBiomeIndex( int inX, int inY,
							 int *outSecondPlaceIndex = nullptr,
							 double *outSecondPlaceGap = nullptr);

int biomeDBGet( int inX, int inY,
				int *outSecondPlaceBiome = nullptr,
				double *outSecondPlaceGap = nullptr);

#endif //OPENLIFE_WORLDMAP_H
