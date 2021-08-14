//
// Created by olivier on 12/08/2021.
//

#ifndef OPENLIFE_COMMON_TYPE_SETTINGS_LINEARDB
#define OPENLIFE_COMMON_TYPE_SETTINGS_LINEARDB

#include <string>

namespace common::type::settings
{
	typedef struct{
		std::string filename;
		unsigned char keySize;
		unsigned char valueSize;
	}LinearDB;
}

#endif //OPENLIFE_COMMON_TYPE_SETTINGS_LINEARDB
