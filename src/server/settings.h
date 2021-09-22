//
// Created by olivier on 17/09/2021.
//

#ifndef OPENLIFE_SERVER_SETTINGS_H
#define OPENLIFE_SERVER_SETTINGS_H

#include <string>
#include <vector>

namespace openLife::server
{
	typedef struct{
		struct{
			std::vector<unsigned int> relief;
		}map;
		struct{
			int type;
			struct{
				std::string filename;
			}sketch;
		}mapGenerator;
	}Settings;
}

#endif //OPENLIFE_SERVER_SETTINGS_H
