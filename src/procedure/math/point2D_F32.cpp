//
// Created by olivier on 13/08/2022.
//

#include "point2D_F32.h"

#include <cmath>

float openLife::procedure::math::point2D_F32::getDistance(
		openLife::dataType::geometric::Point2D_F32 a,
		openLife::dataType::geometric::Point2D_F32 b)
{
	return std::sqrt(((a.x-b.x)*(a.x-b.x))+((a.y-b.y)*(a.y-b.y)));
}