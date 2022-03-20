//
// Created by olivier on 30/08/2021.
//

#ifndef OPENLIFE_EXTENSION_NLOHMANN_H
#define OPENLIFE_EXTENSION_NLOHMANN_H

#include "../../json/single_include/nlohmann/json.hpp"

using json = nlohmann::json;

namespace openLife::extension::nlohmann
{
	json getJsonObject(const char* filename);
}

#endif //OPENLIFE_EXTENSION_NLOHMANN_H
