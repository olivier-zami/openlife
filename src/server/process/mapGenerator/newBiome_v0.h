//
// Created by olivier on 17/09/2021.
//

#ifndef OPENLIFE_NEWBIOME_V0_H
#define OPENLIFE_NEWBIOME_V0_H

#include <vector>
#include <string>
#include "src/system/_base/object/store/memory/extendedVector2D.h"
#include "src/system/_base/type.h"
#include "src/system/_base/type/record.h"

namespace openLife::server::process
{
	openLife::system::type::record::Biome newBiome_v0(
			openLife::system::type::Value2D_32 position,
			openLife::system::type::Value2D_32 localMapPosition,
			std::string filename,
			openLife::system::object::store::memory::ExtendedVector2D<openLife::system::type::record::Biome>* dbBiomeCache
			);
}

#endif //OPENLIFE_NEWBIOME_V0_H
