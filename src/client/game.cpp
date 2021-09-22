//
// Created by olivier on 22/09/2021.
//

#include "game.h"

#include <cstdio>

client::Game::Game()
{
	this->spriteBank = new client::component::bank::Sprite();
}

client::Game::~Game() {}

client::component::bank::Sprite* client::Game::getSprites()
{
	return this->spriteBank;
}