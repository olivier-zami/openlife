//
// Created by olivier on 09/08/2021.
//

#ifndef OPENLIFE_LINEARDB_H
#define OPENLIFE_LINEARDB_H

#include "src/common/type/database/lineardb3.h"

namespace common::object::store::memory::randomAccess
{
	class LinearDB
	{
		public:
			LinearDB(LINEARDB3* dbState);
			~LinearDB();

			void put(int idx, int value);
			int get(int idx);

		private:
			LINEARDB3* dbState;
	};
}

int getMapBiomeIndex( int inX, int inY,
					  int *outSecondPlaceIndex = nullptr,
					  double *outSecondPlaceGap = nullptr);

int biomeDBGet( int inX, int inY,
				int *outSecondPlaceBiome = nullptr,
				double *outSecondPlaceGap = nullptr);

#endif //OPENLIFE_LINEARDB_H
