//
// Created by olivier on 22/09/2021.
//

#ifndef OPENLIFE_GROUND_H
#define OPENLIFE_GROUND_H

#include "minorGems/util/SimpleVector.h"
#include "src/client/component/bank/sprite/ground.h"
#include "minorGems/io/file/File.h"
#include "src/system/_base/process/scalar.h"
#define CELL_D 128

#include "minorGems/game/gameGraphics.h"
typedef struct GroundSpriteSet {
	int biome;

	int numTilesHigh;
	int numTilesWide;

	// indexed as [y][x]
	SpriteHandle **tiles;

	SpriteHandle **squareTiles;

	// all tiles together in one image
	SpriteHandle wholeSheet;
} GroundSpriteSet;

namespace client::component::bank::sprite
{
	class Ground
	{
		public:
			Ground(
					int blurRadius,
					GroundSpriteSet **groundSprites,
					int* groundSpritesArraySize,
					openLife::system::type::Value2D_U32 mapGenSeed);
			~Ground();

			int reset(char inPrintSteps);
			float create();
			void close();//TODO: put this in destroy method

		private:
			SimpleVector<int>* allBiomes;

			// array sized for largest biome ID for direct indexing
			// sparse, with NULL entries
			int* groundSpritesArraySize;
			GroundSpriteSet **groundSprites;
			int nextStep;
			int blurRadius;
			File* groundDir;
			File* groundTileCacheDir;
			char printSteps;
			openLife::system::type::Value2D_U32 mapGenSeed;
	};
}

float initGroundSpritesStep();

// object bank must be inited first

// loads from objects folder
// returns number of ground tiles that need to be loaded
int initGroundSpritesStart( char inPrintSteps=true );

// returns progress... ready for Finish when progress == 1.0
float initGroundSpritesStep();
void initGroundSpritesFinish();

void freeGroundSprites();


#endif //OPENLIFE_GROUND_H
