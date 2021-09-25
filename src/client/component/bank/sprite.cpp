//
// Created by olivier on 22/09/2021.
//

#include "sprite.h"

#include <cstdio>
#include "minorGems/util/SettingsManager.h"

// array sized for largest biome ID for direct indexing
// sparse, with NULL entries
extern GroundSpriteSet **groundSprites;
extern int groundSpritesArraySize;

extern openLife::system::type::Value2D_U32 mapGenSeed;

client::component::bank::Sprite::Sprite()
{
	this->groundBank = new client::component::bank::sprite::Ground(
			SettingsManager::getIntSetting( "groundTileEdgeBlurRadius", 12 ),
			groundSprites,
			&groundSpritesArraySize,
			mapGenSeed);
}

client::component::bank::Sprite::~Sprite() {}

client::component::bank::sprite::Ground* client::component::bank::Sprite::getGrounds()
{
	return this->groundBank;
}