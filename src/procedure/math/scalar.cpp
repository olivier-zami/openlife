//
// Created by olivier on 28/08/2021.
//

#include "scalar.h"

#include <cmath>
#include "src/system/_base/constant.h"

double oneOverIntMax = 1.0 / ( (double)4294967295U );

double openLife::system::process::scalar::getXYFractal( int inX, int inY, double inRoughness, double inScale, openLife::system::type::Value2D_U32 mapGenSeed)
{

	double b = inRoughness;
	double a = 1 - b;

	double sum =
			a * getXYRandomBN( inX / (32 * inScale), inY / (32 * inScale), mapGenSeed )
			+
			b * (
					a * getXYRandomBN( inX / (16 * inScale), inY / (16 * inScale), mapGenSeed )
					+
					b * (
							a * getXYRandomBN( inX / (8 * inScale),
											   inY / (8 * inScale), mapGenSeed )
											   +
											   b * (
											   		a * getXYRandomBN( inX / (4 * inScale),
																		  inY / (4 * inScale), mapGenSeed )
																		  +
																		  b * (
																		  		a * getXYRandomBN( inX / (2 * inScale),
																									 inY / (2 * inScale), mapGenSeed )
																									 +
																									 b * (
																									 		getXYRandomBN( inX / inScale, inY / inScale, mapGenSeed )
																									 		) ) ) ) );

	return sum * oneOverIntMax;
}

// in 0..uintMax
// interpolated for inX,inY that aren't integers
double openLife::system::process::scalar::getXYRandomBN( double inX, double inY, openLife::system::type::Value2D_U32 mapGenSeed )
{
	int floorX = lrint( floor(inX) );
	int ceilX = floorX + 1;
	int floorY = lrint( floor(inY) );
	int ceilY = floorY + 1;

	double cornerA1 = openLife::system::process::scalar::xxTweakedHash2D( floorX, floorY, mapGenSeed.x , mapGenSeed.y);
	double cornerA2 = openLife::system::process::scalar::xxTweakedHash2D( ceilX, floorY, mapGenSeed.x , mapGenSeed.y );

	double cornerB1 = openLife::system::process::scalar::xxTweakedHash2D( floorX, ceilY, mapGenSeed.x , mapGenSeed.y );
	double cornerB2 = openLife::system::process::scalar::xxTweakedHash2D( ceilX, ceilY, mapGenSeed.x , mapGenSeed.y );

	double xOffset = inX - floorX;
	double yOffset = inY - floorY;

	double topBlend = cornerA2 * xOffset + (1-xOffset) * cornerA1;
	double bottomBlend = cornerB2 * xOffset + (1-xOffset) * cornerB1;

	return bottomBlend * yOffset + (1-yOffset) * topBlend;
}

// tweaked to be faster by removing lines that don't seem to matter
// for procedural content generation
uint32_t openLife::system::process::scalar::xxTweakedHash2D( uint32_t inX, uint32_t inY, uint32_t xxSeedA, uint32_t xxSeedB)
{
	uint32_t h32 = xxSeedA + inX + XX_PRIME32_5;
	//h32 += 4U;
	h32 += inY * XX_PRIME32_3;
	//h32 = XX_ROTATE_LEFT( h32, 17 ) * XX_PRIME32_4;
	//h32 ^= h32 >> 15;
	h32 *= XX_PRIME32_2;
	h32 ^= h32 >> 13;
	h32 += xxSeedB;
	h32 *= XX_PRIME32_3;
	h32 ^= h32 >> 16;
	return h32;
}