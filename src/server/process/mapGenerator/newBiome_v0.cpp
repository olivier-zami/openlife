//
// Created by olivier on 17/09/2021.
//

#include <iostream>
#include "newBiome_v0.h"
#include "src/system/_base/object/process/handler/image.h"

openLife::system::type::record::Biome openLife::server::process::newBiome_v0(
		int posX, int posY,
		std::string filename,
		openLife::system::object::store::memory::random::Biome* dbCacheBiome)
{
	struct{
		int x;
		int y;
	}sketchPosition = {0, 0};
	openLife::system::type::record::Biome biome = {posX, posY, -1, 0, 0};
	openLife::system::object::process::handler::Image* imageHandler = new openLife::system::object::process::handler::Image();
	imageHandler->load(filename.c_str());
	Image image = imageHandler->getImageInfo();
	std::cout << "\n###############>Current pos("<<posX<<", "<<posY<<")";
	if((posX>=sketchPosition.x&&posX<sketchPosition.x+(int)image.width)&&(posY>=sketchPosition.y&&posY<sketchPosition.y+(int)image.height))
	{
		std::cout << "\n=============> Generate biome from ("<<sketchPosition.x<<", "<<sketchPosition.y<<") to ("<<(sketchPosition.x+image.width)<<", "<<(sketchPosition.y+image.height)<<")";
		for(int y=sketchPosition.y; y<sketchPosition.y+(int)image.height; y++)
		{
			for(int x=sketchPosition.x; x<sketchPosition.x+(int)image.width; x++)
			{
				int groundType = 1;
				ColorRGB pixel = imageHandler->select((openLife::system::type::geometric::Point2D_32){(int)x, (int)y})->getPixel();
				if(pixel.r==0&&pixel.g==0&&pixel.b==255)groundType = 0;
				if(pixel.r==0&&pixel.g==127&&pixel.b==127)groundType = 0;//swamp
				if(pixel.r==0&&pixel.g==255&&pixel.b==0)groundType = 1;//grassland
				if(pixel.r==255&&pixel.g==127&&pixel.b==0)groundType = 2;//polar
				if(pixel.r==127&&pixel.g==127&&pixel.b==127)groundType = 3;//moutain
				if(pixel.r==255&&pixel.g==255&&pixel.b==0)groundType = 5;//desert
				if(pixel.r==0&&pixel.g==127&&pixel.b==0)groundType = 5;//jungle
				if(pixel.r==255&&pixel.g==255&&pixel.b==255)groundType = 6;//polar

				dbCacheBiome->put((openLife::system::type::record::Biome){x, y, groundType, 0, 0});
				if(x==posX && y==posY) biome = {x, y, groundType, 0, 0};
			}
		}
	}
	else biome = {posX, posY, 1, 0, 0};

	return biome;
}