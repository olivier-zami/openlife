#include <stdarg.h>
#include <math.h>

#include "src/system/_base/constant.h"
#include "src/system/_base/process/scalar.h"

#include "fractalNoise.h"

extern double oneOverIntMax;
extern openLife::system::type::Value2D_U32 mapGenSeed;

static uint32_t xxSeedA = 0U;
static uint32_t xxSeedB = 0U;



void setXYRandomSeed( uint32_t inSeedA, uint32_t inSeedB )
{
    xxSeedA = inSeedA;
    mapGenSeed.x = inSeedA;
    xxSeedB = inSeedB;
    mapGenSeed.y = inSeedB;
}



#define XX_ROTATE_LEFT( inValue, inCount ) \
    ( (inValue << inCount) | (inValue >> (32 - inCount) ) )
      

// original XX hash algorithm as found here:
// https://bitbucket.org/runevision/random-numbers-testing/
/*
static uint32_t xxHash( uint32_t inValue ) {
    uint32_t h32 = xxSeed + XX_PRIME32_5;
    h32 += 4U;
    h32 += inValue * XX_PRIME32_3;
    h32 = XX_ROTATE_LEFT( h32, 17 ) * XX_PRIME32_4;
    h32 ^= h32 >> 15;
    h32 *= XX_PRIME32_2;
    h32 ^= h32 >> 13;
    h32 *= XX_PRIME32_3;
    h32 ^= h32 >> 16;
    return h32;
    }

// modified xxHash to take two int arguments
static uint32_t xxHash2D( uint32_t inX, uint32_t inY ) {
    uint32_t h32 = xxSeed + inX + XX_PRIME32_5;
    h32 += 4U;
    h32 += inY * XX_PRIME32_3;
    h32 = XX_ROTATE_LEFT( h32, 17 ) * XX_PRIME32_4;
    h32 ^= h32 >> 15;
    h32 *= XX_PRIME32_2;
    h32 ^= h32 >> 13;
    h32 *= XX_PRIME32_3;
    h32 ^= h32 >> 16;
    return h32;
    }
*/


double getXYRandom( int inX, int inY ) {
	return openLife::system::process::scalar::xxTweakedHash2D( inX, inY, xxSeedA, xxSeedB ) * oneOverIntMax;
    }



