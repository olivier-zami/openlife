//
// Created by olivier on 11/08/2021.
//

//
// Created by olivier on 07/02/2021.
//

#include "image.h"

#include <SDL/SDL.h>
#include <iostream>
#include "src/common/object/entity/mapZone.h"

/*

#include <cstdlib>
#include <cstring>
#include "SDL.h"
#include "map.h"
*/
common::object::entity::MapZone* common::process::convert::image::getMapZoneFromBitmap(const char *filename)
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

common::type::color::RGB common::process::convert::image::getRGBFromInt(int rgbColor)
{
	common::type::color::RGB color;
	color.b = rgbColor & 255;
	color.g = (rgbColor >> 8) & 255;
	color.r = (rgbColor >> 16) & 255;
	return color;
}

