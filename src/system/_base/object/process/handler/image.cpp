//
// Created by olivier on 11/08/2021.
//

//
// Created by olivier on 07/02/2021.
//

#include "image.h"

#include <iostream>

/*
#include "src/common/object/entity/mapZone.h"
*/

openLife::system::object::process::handler::Image::Image()
{
	this->surface = nullptr;
	this->bytePtr = nullptr;
}

openLife::system::object::process::handler::Image::~Image()
{
	if(this->surface) SDL_FreeSurface(this->surface);
}

void openLife::system::object::process::handler::Image::create(int width, int height, int depth, Uint32 rMask, Uint32 gMask, Uint32 bMask, Uint32 aMask)
{

	this->surface = SDL_CreateRGBSurface(0, width, height, depth, rMask, gMask, bMask, aMask);
	if(!this->surface){std::cout << "SDL_CreateRGBSurface() failed: " << SDL_GetError(); exit(1);}
	this->initImageInfo();
}

void openLife::system::object::process::handler::Image::load(const char* filename)
{
	this->surface = SDL_LoadBMP(filename);
	if(!this->surface) {std::cout << SDL_GetError() << "\n";exit(1);}
	this->initImageInfo();
}

void openLife::system::object::process::handler::Image::save(const char* filename)
{
	if(!this->surface) {std::cout << "\nno image set "; exit(1);}
	if(SDL_SaveBMP(this->surface, filename) != 0) std::cout << "\nSDL_SaveBMP failed: " << SDL_GetError();
}

Image openLife::system::object::process::handler::Image::getImageInfo()
{
	return this->imageInfo;
}

openLife::system::object::process::handler::Image* openLife::system::object::process::handler::Image::select(unsigned int x, unsigned int y)
{
	this->query.pixelIdx = x + (this->imageInfo.width * y);
	return this;
}

void openLife::system::object::process::handler::Image::setPixel(ColorRGB color)
{
	unsigned int idx = this->query.pixelIdx * this->imageInfo.bytesPerPixel;
	this->bytePtr[idx] = (unsigned char)color.b;
	this->bytePtr[idx+1] = (unsigned char)color.g;
	this->bytePtr[idx+2] = (unsigned char)color.r;
}

void openLife::system::object::process::handler::Image::clean()//TODO: test validity of image before
{
	if(!this->surface) {std::cout << "\nno image set "; exit(1);}
	unsigned char *ptr;
	ptr = this->bytePtr;
	for(unsigned int i=0; i<this->imageInfo.pixelNumber; i++)
	{
		ptr[0] = 0; ptr[1] = 0; ptr[2] = 0;
		ptr+=this->imageInfo.bytesPerPixel;
	}
}

void openLife::system::object::process::handler::Image::initImageInfo()
{
	this->imageInfo.bytesPerPixel = this->surface->format->BytesPerPixel;
	this->imageInfo.width = this->surface->w;
	this->imageInfo.height = this->surface->h;
	this->imageInfo.pixelNumber = this->surface->w * this->surface->h;
	this->bytePtr = (unsigned char*)this->surface->pixels;
}

/*
common::object::entity::MapZone* openLife::system::object::process::converter::image::getMapZoneFromBitmap(const char *filename)
{
	SDL_Surface *sMap = nullptr;
	sMap = SDL_LoadBMP(filename);
	if(!sMap) {std::cout << SDL_GetError() << "\n";exit(1);}

	common::object::entity::MapZone* mapZone;
	mapZone = new common::object::entity::MapZone((unsigned int)sMap->w, (unsigned int)sMap->h);
	unsigned int bytePerPixel = sMap->format->BytesPerPixel;
	unsigned int pixelNumber = sMap->w * sMap->h;
	unsigned char* byte=(unsigned char*)sMap->pixels;
	for(unsigned int i=0; i<pixelNumber; i++)
	{
		mapZone->p(i) = byte[0] | byte[1] << 8 | byte[2] << 16;
		byte+=bytePerPixel;
	}
	if(sMap)SDL_FreeSurface(sMap);

	return mapZone;
}

common::type::color::RGB openLife::system::object::process::converter::image::getRGBFromInt(int rgbColor)
{
	common::type::color::RGB color;
	color.b = rgbColor & 255;
	color.g = (rgbColor >> 8) & 255;
	color.r = (rgbColor >> 16) & 255;
	return color;
}
*/
