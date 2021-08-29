//
// Created by olivier on 28/08/2021.
//

#ifndef OPENLIFE_SYSTEM_PROCESS_SCALAR_H
#define OPENLIFE_SYSTEM_PROCESS_SCALAR_H

#include <cstdint>
#include "src/system/_base/type.h"

namespace openLife::system::process::scalar
{
	double getXYFractal( int inX, int inY, double inRoughness, double inScale, openLife::system::type::Value2D_U32 mapGenSeed);
	double getXYRandomBN( double inX, double inY, openLife::system::type::Value2D_U32 mapGenSeed);
	uint32_t xxTweakedHash2D( uint32_t inX, uint32_t inY, uint32_t xxSeedA = 0U, uint32_t xxSeedB = 0U);
}

#endif //OPENLIFE_SYSTEM_PROCESS_SCALAR_H
