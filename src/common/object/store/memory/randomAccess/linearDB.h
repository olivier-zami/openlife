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
			int get(unsigned char idx[8], unsigned char value[12]);

		private:
			LINEARDB3* dbState;
	};
}
#endif //OPENLIFE_LINEARDB_H
