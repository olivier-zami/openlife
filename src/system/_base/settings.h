//
// Created by olivier on 15/09/2021.
//

#ifndef OPENLIFE_SYSTEM_SETTINGS_H
#define OPENLIFE_SYSTEM_SETTINGS_H

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

	typedef struct{
		unsigned int size;
	}RandomAccessStore;
}

#endif //OPENLIFE_SYSTEM_SETTINGS_H
