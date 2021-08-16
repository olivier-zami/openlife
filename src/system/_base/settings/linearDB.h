//
// Created by olivier on 12/08/2021.
//

#ifndef OPENLIFE_COMMON_TYPE_SETTINGS_LINEARDB
#define OPENLIFE_COMMON_TYPE_SETTINGS_LINEARDB

#include <string>

namespace openLife::system::settings
{
	typedef struct{
		std::string filename;
		struct{
			unsigned int keySize;
			unsigned int valueSize;
		}record;
	}LinearDB;
}

#endif //OPENLIFE_COMMON_TYPE_SETTINGS_LINEARDB
