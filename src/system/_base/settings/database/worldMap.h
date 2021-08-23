//
// Created by olivier on 23/08/2021.
//

#ifndef OPENLIFE_SYSTEM_SETTINGS_DATABASE_WORLDMAP_H
#define OPENLIFE_SYSTEM_SETTINGS_DATABASE_WORLDMAP_H

#include "src/system/_base/type.h"

namespace openLife::system::settings::database
{
	typedef struct{
		std::string filename;
		openLife::system::type::Dimension2D mapSize;

	}WorldMap;
}

#endif //OPENLIFE_SYSTEM_SETTINGS_DATABASE_WORLDMAP_H
