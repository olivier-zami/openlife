//
// Created by olivier on 11/08/2021.
//

#ifndef ONELIFETEST_IMAGE_H
#define ONELIFETEST_IMAGE_H

#include "src/common/object/entity/mapZone.h"
#include "src/common/type/color.h"

namespace common::process::convert::image
{
	common::object::entity::MapZone* getMapZoneFromBitmap(const char *filename);
	common::type::color::RGB getRGBFromInt(int rgbColor);
}


#endif //ONELIFETEST_IMAGE_H
