//
// Created by olivier on 22/09/2021.
//

#include "sprite.h"

#include <cstdio>

client::component::bank::Sprite::Sprite()
{
	this->groundBank = new client::component::bank::sprite::Ground();
}

client::component::bank::Sprite::~Sprite() {}

client::component::bank::sprite::Ground* client::component::bank::Sprite::getGround()
{
	return this->groundBank;
}