//
// Created by olivier on 30/08/2021.
//

#ifndef OPENLIFE_SYSTEM_TYPE_ENTITY_H
#define OPENLIFE_SYSTEM_TYPE_ENTITY_H

#include <string>

namespace openLife::server::type::entity
{
	typedef struct{
		std::string label;
		int code;
		int value;
	}Biome;

	typedef struct{
		std::string label;
	}Climate;
}

#endif //OPENLIFE_SYSTEM_TYPE_ENTITY_H
