//
// Created by olivier on 17/09/2021.
//

#include <iostream>
#include "newBiome_v0.h"
#include "src/system/_base/object/process/handler/image.h"

openLife::system::type::record::Biome openLife::server::process::newBiome_v0(
		openLife::system::type::Value2D_32 position,
		openLife::system::type::Value2D_32 localMapPosition,
		std::string filename,
		openLife::system::object::store::memory::ExtendedVector2D<openLife::system::type::record::Biome>* dbBiomeCache)
{
	if(!dbBiomeCache->getTotalSize())
	{
		openLife::system::object::process::handler::Image* imageHandler = new openLife::system::object::process::handler::Image();
		imageHandler->load(filename.c_str());
		Image image = imageHandler->getImageInfo();
		dbBiomeCache->reserve(image.width, image.height);
		dbBiomeCache->setDefaultValue({0, 0, -1, 0, 0});
		dbBiomeCache->reset();

		for(unsigned int y=0; y<image.height; y++)
		{
			for(unsigned int x=0; x<image.width; x++)
			{
				int groundType = -1;
				ColorRGB pixel = imageHandler->select((openLife::system::type::geometric::Point2D_32){(int)x, (int)(image.width-(y+1))})->getPixel();
				if(pixel.r==0&&pixel.g==0&&pixel.b==255)groundType = 0;//water
				if(pixel.r==0&&pixel.g==127&&pixel.b==127)groundType = 0;//swamp
				if(pixel.r==0&&pixel.g==255&&pixel.b==0)groundType = 1;//grassland
				if(pixel.r==255&&pixel.g==127&&pixel.b==0)groundType = 2;//polar
				if(pixel.r==127&&pixel.g==127&&pixel.b==127)groundType = 3;//moutain
				if(pixel.r==255&&pixel.g==255&&pixel.b==0)groundType = 5;//desert
				if(pixel.r==0&&pixel.g==127&&pixel.b==0)groundType = 5;//jungle
				if(pixel.r==255&&pixel.g==255&&pixel.b==255)groundType = 6;//polar
				dbBiomeCache->set({x, y}, {(int)(localMapPosition.x+x), (int)(localMapPosition.y+y), groundType, 0, 0});
			}
		}
	}

	openLife::system::type::record::Biome biome;
	openLife::system::type::Value2D_U32 mapSize = dbBiomeCache->getSize();
	if((position.x>=localMapPosition.x&&position.x<localMapPosition.x+(int)mapSize.x)&&(position.y>=localMapPosition.y&&position.y<localMapPosition.y+(int)mapSize.y))
	{
		biome = dbBiomeCache->get(position.x-localMapPosition.x, position.y-localMapPosition.y);
	}
	else biome = {position.x, position.y, -1, 0, 0};

	return biome;
}