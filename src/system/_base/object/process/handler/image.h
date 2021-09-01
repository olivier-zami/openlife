//
// Created by olivier on 11/08/2021.
//

#ifndef OPENLIFE_SYSTEM_OBJECT_PROCESS_CONVERT_IMAGE_H
#define OPENLIFE_SYSTEM_OBJECT_PROCESS_CONVERT_IMAGE_H

#include <SDL/SDL.h>
/*
#include "src/common/object/entity/mapZone.h"
#include "src/common/type/color.h"
*/

typedef struct
{
	unsigned int width;
	unsigned int height;
	unsigned int bytesPerPixel;
	unsigned int pixelNumber;
}Image;

typedef struct
{
	unsigned int r;
	unsigned int g;
	unsigned int b;
}ColorRGB;

namespace openLife::system::object::process::handler
{
	class Image
	{
		public:
			Image();
			~Image();

			void create(int width, int height, int depth=32, Uint32 rMask=0, Uint32 gMask=0, Uint32 bMask=0, Uint32 aMask=0);
			void load(const char* filename);
			void save(const char* filename);
			::Image getImageInfo();

			openLife::system::object::process::handler::Image* select(unsigned int x, unsigned int y);
			void setPixel(ColorRGB color);
			void clean();

		private:
			void initImageInfo();
			SDL_Surface* surface = nullptr;
			::Image imageInfo;
			unsigned char* bytePtr;
			struct{
				unsigned int pixelIdx;
			}query;
	};
}


#endif //OPENLIFE_SYSTEM_OBJECT_PROCESS_CONVERT_IMAGE_H
