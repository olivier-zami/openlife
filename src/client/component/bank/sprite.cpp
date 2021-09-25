//
// Created by olivier on 22/09/2021.
//

#include "sprite.h"

extern openLife::system::type::Value2D_U32 mapGenSeed;

client::component::bank::Sprite::Sprite()
{
	this->groundBank = new client::component::bank::sprite::Ground();
}

client::component::bank::Sprite::~Sprite() {}

client::component::bank::sprite::Ground* client::component::bank::Sprite::getGrounds()
{
	return this->groundBank;
}