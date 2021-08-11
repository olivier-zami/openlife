//
// Created by olivier on 09/08/2021.
//

#ifndef OPENLIFE_LINEARDB_H
#define OPENLIFE_LINEARDB_H

namespace common::object::store::memory::randomAccess
{
	class LinearDB
	{
		public:
			LinearDB();
			~LinearDB();

			void put(int idx, int value);
			int get(int idx);
	};
}

#endif //OPENLIFE_LINEARDB_H
