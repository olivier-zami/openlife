//
// Created by olivier on 10/08/2021.
//

#include "mapZone.h"

common::object::entity::MapZone::MapZone(unsigned int width, unsigned int height)
{
	this->width = width;
	this->height = height;
	unsigned long int size = this->width*this->height;
	this->coord.reserve(size);
}

common::object::entity::MapZone::~MapZone() {}

int &common::object::entity::MapZone::p(unsigned int idx)
{
	return this->coord[idx];
}

int &common::object::entity::MapZone::p(unsigned int x, unsigned int y)
{
	x = (x > this->width-1) ? this->width-1 : x;
	y = (y > this->height-1) ? this->height-1 : y;
	return this->coord[x+(this->width*y)];
}

unsigned int common::object::entity::MapZone::getSize()
{
	return this->coord.capacity();
}

unsigned int common::object::entity::MapZone::getWidth()
{
	return this->width;
}

unsigned int common::object::entity::MapZone::getHeight()
{
	return this->height;
}
