//
// Created by olivier on 02/09/2021.
//

#ifndef OPENLIFE_GETCOASTLINESHAPE_H
#define OPENLIFE_GETCOASTLINESHAPE_H

#include <vector>
#include "src/system/_base/type.h"
#include "src/system/_base/type/geometric.h"

namespace openLife::system::process::container::point2D
{
	typedef struct{
		openLife::dataType::geometric::Point2D_32 center;
		struct{
			unsigned int number;
		}initialPoint;
		struct{
			unsigned int iteration;
		}process;
	}coastalShapeSettings;
	std::vector<openLife::dataType::geometric::Point2D_32> getCoastalShape(coastalShapeSettings settings);
}

#endif //OPENLIFE_GETCOASTLINESHAPE_H
