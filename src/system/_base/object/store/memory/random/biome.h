//
// Created by olivier on 26/08/2021.
//

#ifndef OPENLIFE_SYSTEM_OBJECT_STORE_MEMORY_RANDOM_BIOME_H
#define OPENLIFE_SYSTEM_OBJECT_STORE_MEMORY_RANDOM_BIOME_H

#include <vector>
#include "src/system/_base/type/record.h"

namespace openLife::system::object::store::memory::random
{
	class Biome
	{
		public:
			Biome(unsigned int size);
			~Biome();

			void put(int idx, openLife::system::type::record::Biome record);
			void put (openLife::system::type::record::Biome record);
			openLife::system::type::record::Biome get(int idx);
			openLife::system::type::record::Biome get(int x, int y);

		private:
			int generateHashKey(int x, int y);
			std::vector<openLife::system::type::record::Biome> biome;
	};
}


#endif //OPENLIFE_SYSTEM_OBJECT_STORE_MEMORY_RANDOM_BIOME_H
