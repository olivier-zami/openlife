//
// Created by olivier on 27/08/2021.
//


#include <iostream>
#include <vector>

#include "src/server/process/mapGenerator/newBiome_v0.h"
#include "src/server/process/newBiome_v1.h"
#include "src/server/type/entities.h"
#include "src/system/_base/process/scalar.h"
#include "src/system/_base/object/process/handler/image.h"
#include "src/system/_base/object/store/memory/extendedVector2D.h"
#include "src/system/_extension/nlohmann.h"

#define BIOME_CACHE_SIZE 131072

/*
void initSettings()
{
	//!load config file
	nlohmann::json data = openLife::system::nlohmann::getJsonFromFile("app/tools/mapDrawer.json");
	nlohmann::json dataClimate = openLife::system::nlohmann::getJsonFromFile("conf/entity/climate.json");
	openLife::system::type::Value2D_U32 randSeed = {data["map"]["seed"]["x"], data["map"]["seed"]["y"]};
	char allowSecondPlaceBiomes = data["map"]["allowSecondPlaceBiomes"].get<char>();
	std::vector<int> biomes;
	for(unsigned int i=0; i<data["map"]["biomeOrder"].size(); i++) biomes.push_back(data["map"]["biomeOrder"][i]);
	std::vector<int> specialBiomes;
	for(unsigned int i=0; i<data["map"]["specialBiomes"].size(); i++) specialBiomes.push_back(data["map"]["specialBiomes"][i]);
	std::vector<float> biomeWeights;
	for(unsigned int i=0; i<data["map"]["biomeWeights"].size(); i++) biomeWeights.push_back(data["map"]["biomeWeights"][i]);

	int specialBiomeBandMode = data["map"]["specialBiomeBandMode"];
	int specialBiomeBandThickness = data["map"]["specialBiomeBandThickness"];
	std::vector<int> specialBiomeBandYCenter;
	for(unsigned int i=0; i<data["map"]["specialBiomeBandYCenter"].size(); i++) specialBiomeBandYCenter.push_back(data["map"]["specialBiomeBandYCenter"][i]);
	std::vector<int> specialBiomeBandIndexOrder;
	for(unsigned int i=0; i<data["map"]["specialBiomeBandIndexOrder"].size(); i++) specialBiomeBandIndexOrder.push_back(data["map"]["specialBiomeBandIndexOrder"][i]);
}
*/

int main()
{
	std::cout << "\nStarting MapDrawer Tool ...";

	/*
	//!
	openLife::system::type::Value2D_32 minMapLimit = {(int32_t)mapCenter.x-(int32_t)mapSize.x, (int32_t)mapCenter.y-(int32_t)mapSize.y};
	openLife::system::type::Value2D_32 maxMapLimit = {(int32_t)mapSize.x-(int32_t)mapCenter.x, (int32_t)mapSize.y-(int32_t)mapCenter.y};

	//imageHandler->load("/home/olivier/Projets/OpenLife/data/images/maps/mini_map.bmp");
	Image image = imageHandler->getImageInfo();
	std::cout << "\nModification image de dimension ("<<image.width<<","<<image.height<<")";
	std::cout << "\nbytes per pix : " << image.bytesPerPixel;
	std::cout << "\nnumber of pix : " << image.pixelNumber;

	struct{
		int x;
		int y;
	}topLeftCorner = {512-64, 256-64};

	std::cout << "\nymin : " << minMapLimit.y;
	*/

	/*
	for(int y=minMapLimit.y; y<maxMapLimit.y; y++)
	{
		for(int x=minMapLimit.x; x<maxMapLimit.x; x++)
		{
			//openLife::system::type::record::Biome pickedBiome = {x, y, 2, 0, 0};

			if(x>=topLeftCorner.x&&x<(topLeftCorner.x+128)&&y>=topLeftCorner.y&&y<topLeftCorner.y+128)
			{
				pickedBiome = cache->get(x,y);
			}

			/ *
			if(pickedBiome.value == -1)
			{
				pickedBiome = openLife::server::process::newBiome_v0(
						x, y,
						sketch,
						cache);
			}
			* /

			/ *
			openLife::system::type::record::Biome pickedBiome = openLife::server::process::newBiome_v1(
				x, y,
				randSeed,
				allowSecondPlaceBiomes,
				biomes,
				specialBiomeBandThickness,
				specialBiomeBandYCenter,
				specialBiomeBandIndexOrder,
				specialBiomeBandMode,
				specialBiomes,
				biomeWeights);
			 * /
			int imgX = (image.width/2)+x;
			int imgY = (image.height/2)-y;
			if(imgX>=0&&imgX<(int)mapSize.x&&imgY>=0&&imgY<(int)mapSize.y) imageHandler->select((openLife::system::type::geometric::Point2D_32){imgX, imgY})->setPixel(color);
		}
	}
	 */

	//!
	int mapGeneratorType = 1;
	openLife::system::type::Value2D_U32 mapSize = {1024, 512};
	openLife::system::type::Value2D_U32 mapCenter = {mapSize.x/2, mapSize.y/2};
	std::cout << "\nCreate picture size("<<mapSize.x<<", "<<mapSize.y<<") center("<<mapCenter.x<<", "<<mapCenter.y<<")";

	//!Image creation
	openLife::system::object::process::handler::Image* imageHandler = new openLife::system::object::process::handler::Image();
	imageHandler->create(mapSize.x, mapSize.y);

	//!MapGenerator Setting
	openLife::system::object::store::memory::ExtendedVector2D<openLife::system::type::record::Biome>* cache;
	cache = new openLife::system::object::store::memory::ExtendedVector2D<openLife::system::type::record::Biome>();
	std::string sketchFilename("/home/olivier/Projets/OpenLife/data/images/maps/mini_map.bmp");



	//!Image painting
	openLife::system::type::record::Biome pickedBiome;
	for(unsigned int y=0; y<mapSize.y; y++)
	{
		for(unsigned int x=0; x<mapSize.x; x++)
		{
			//!pick element from mapGenerator
			switch(mapGeneratorType)
			{
				case 1:
					pickedBiome = openLife::server::process::newBiome_v0(
							(openLife::system::type::Value2D_32){(int)(x-mapCenter.x), (int)(y-mapCenter.y)},
							(openLife::system::type::Value2D_32){-256, -256},
							sketchFilename,
							cache);
					break;
				default:
					pickedBiome = {(int)x, (int)y, -1, 0, 0};
					break;
			}


			//!set pixel
			ColorRGB color;
			unsigned int offset = (x==mapCenter.x || y==mapCenter.y) ? 50 : 0;
			switch(pickedBiome.value)
			{
				case 0://swamp
				color = {95+offset, 0, 125+offset};
				break;
				case 1://grassland
				color = {0, 190+offset, 0};
				break;
				case 2://savannah
				color = {190+offset, 95+offset, 0};
				break;
				case 3://mountain
				color = {95+offset, 95+offset, 95+offset};
				break;
				case 4://jungle
				color = {0, 95+offset, 0};
				break;
				case 5://desert
				color = {190+offset, 190+offset, 0};
				break;
				case 6://polar
				color = {190+offset, 190+offset, 190+offset};
				break;
				default://unknown
				color = {0+offset, 0+offset, 0+offset};
				break;
			}
			imageHandler->select((openLife::system::type::geometric::Point2D_32){(int)x, (int)y})->setPixel(color);
		}
	}

	imageHandler->save("/home/olivier/Projets/OpenLife/var/images/maps/map_test_00.bmp");
	std::cout << "\nGenerate \"/home/olivier/Projets/OpenLife/var/images/maps/map_test_00.bmp\""  ;

	std::cout << "\n\n";
	return 0;
}

