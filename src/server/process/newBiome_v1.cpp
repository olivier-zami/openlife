//
// Created by olivier on 29/08/2021.
//

#include "newBiome_v1.h"

#include <cstdlib>
#include <iostream>
#include "src/system/_base/process/scalar.h"

openLife::system::type::entity::Biome openLife::server::process::newBiome_v1(
		int x, int y,
		openLife::system::type::Value2D_U32 randSeed,
		char allowSecondPlaceBiomes,
		std::vector<int> biomes,
		int specialBiomeBandThickness,
		std::vector<int> specialBiomeBandYCenter,
		std::vector<int> specialBiomeBandIndexOrder,
		int specialBiomeBandMode,
		std::vector<int> specialBiomes,
		std::vector<float> biomeWeights)
{
	//!legacy : int computeMapBiomeIndex(x, y);
	openLife::system::type::entity::Biome newBiome = {x, y, 0, -1, 0};

	int numSpecialBiomes = specialBiomes.size();
	int numBiomes = biomes.size();
	int regularBiomeLimit = numBiomes - numSpecialBiomes;


	float biomeTotalWeight = 0;
	float *biomeCumuWeights = new float[ numBiomes ];
	for( int i=0; i<numBiomes; i++ ) {
		biomeTotalWeight += biomeWeights[i];
		biomeCumuWeights[i] = biomeTotalWeight;
	}

	// try topographical altitude mapping
	double randVal = openLife::system::process::scalar::getXYFractal(x, y, 0.55, 0.83332 + 0.08333 * numBiomes, randSeed) ;

	// push into range 0..1, based on sampled min/max values
	randVal -= 0.099668;
	randVal *= 1.268963;

	float i = randVal * biomeTotalWeight;
	while( newBiome.value < numBiomes && i > biomeCumuWeights[newBiome.value] ) newBiome.value++;

	if( newBiome.value >= numBiomes ) newBiome.value = numBiomes - 1;

	if( newBiome.value >= regularBiomeLimit && numSpecialBiomes > 0 )
	{
		// special case:  on a peak, place a special biome here
		if( specialBiomeBandMode )
		{
			// use band mode for these
			/***/
			//!legacy: newBiome.value = getSpecialBiomeIndexForYBand( y );
			// new method, use y centers and thickness
			int value = 0;
			int radius = specialBiomeBandThickness / 2;
			for(std::vector<int>::size_type i=0; i<specialBiomeBandYCenter.size(); i++)
			{
				int yCenter = specialBiomeBandYCenter[i];

				if( abs( y - yCenter ) <= radius )
				{
					value = specialBiomeBandIndexOrder[i];
					break;
				}
			}
			newBiome.value = value;
			/****/

			newBiome.secondPlace = regularBiomeLimit - 1;
			newBiome.secondPlaceGap = 0.1;
		}
		else
		{
			// use patches mode for these
			newBiome.value = -1;

			double maxValue = -10;
			double secondMaxVal = -10;

			for( int i=regularBiomeLimit; i<numBiomes; i++ )
			{
				int biome = biomes[i];
				openLife::system::type::Value2D_U32 biomeGenSeed = {biome * 263 + randSeed.x + 38475, randSeed.y};
				double randVal = openLife::system::process::scalar::getXYFractal(  x,
																				   y,
																				   0.55,
																				   2.4999 + 0.2499 * numSpecialBiomes,
																				   biomeGenSeed );

				if( randVal > maxValue )
				{
					if( maxValue != -10 )
					{
						secondMaxVal = maxValue;
					}
					maxValue = randVal;
					newBiome.value = i;
				}
			}

			if( maxValue - secondMaxVal < 0.03 )
			{
				// close!  that means we're on a boundary between special biomes

				// stick last regular biome on this boundary, so special
				// biomes never touch
				newBiome.secondPlace = newBiome.value;
				newBiome.secondPlaceGap = 0.1;
				newBiome.value = regularBiomeLimit - 1;
			}
			else {
				newBiome.secondPlace = regularBiomeLimit - 1;
				newBiome.secondPlaceGap = 0.1;
			}
		}
	}
	else
	{
		// second place for regular biome rings

		newBiome.secondPlace = newBiome.value - 1;
		if( newBiome.secondPlace < 0 ) {
			newBiome.secondPlace = newBiome.value + 1;
		}
		newBiome.secondPlaceGap = 0.1;
	}


	if( ! allowSecondPlaceBiomes ) {
		// make the gap ridiculously big, so that second-place placement
		// never happens.
		// but keep secondPlace set different from pickedBiome
		// (elsewhere in code, we avoid placing animals if
		// secondPlace == picked
		newBiome.secondPlaceGap = 10.0;
	}

	return newBiome;
}
