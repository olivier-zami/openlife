//
// Created by olivier on 10/08/2021.
//

#ifndef OPENLIFE_COMMON_OBJECT_ENTITY_MAPZONE_H
#define OPENLIFE_COMMON_OBJECT_ENTITY_MAPZONE_H

#include <vector>

namespace common::object::entity
{
	class MapZone
	{
		public:
			MapZone(unsigned int width, unsigned int height);
			~MapZone();

			int& p(unsigned int idx);
			int& p(unsigned int width, unsigned int height);
			unsigned int getSize();
			unsigned int getWidth();
			unsigned int getHeight();

		private:
			unsigned int width;
			unsigned int height;
			std::vector<int> coord;
	};
}


#endif //OPENLIFE_COMMON_OBJECT_ENTITY_MAPZONE_H
