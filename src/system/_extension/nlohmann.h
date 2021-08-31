//
// Created by olivier on 30/08/2021.
//

#ifndef OPENLIFE_SYSTEM_NLOHMANN_H
#define OPENLIFE_SYSTEM_NLOHMANN_H

#include "src/third_party/nlohmann/json.hpp"

namespace openLife::system::nlohmann
{
	::nlohmann::json getJsonFromFile(const char* filename);
}

#endif //OPENLIFE_SYSTEM_NLOHMANN_H
