//
// Created by olivier on 11/08/2021.
//

#ifndef OPENLIFE_SYSTEM_OBJECT_PROCESS_CONVERT_IMAGE_H
#define OPENLIFE_SYSTEM_OBJECT_PROCESS_CONVERT_IMAGE_H

#include <SDL/SDL.h>

#include "src/system/_base/type/geometric.h"
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
}Image;//TODO separation Bitmap/ImageHandler

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

			openLife::system::object::process::handler::Image* select(openLife::system::type::geometric::Point2D_32 point);
			openLife::system::object::process::handler::Image* select(openLife::system::type::geometric::Line2D_32 line);
			openLife::system::object::process::handler::Image* select(openLife::system::type::geometric::Circle2D_32 circle);
			void setPixel(ColorRGB color);
			ColorRGB getPixel();
			void clean();

		private:
			void initImageInfo();
			void drawCircle(openLife::system::type::geometric::Circle2D_32 circle, ColorRGB color);
			void drawLine(openLife::system::type::geometric::Line2D_32 line, ColorRGB color);
			void drawPixel(unsigned int idx, ColorRGB color);
			SDL_Surface* surface = nullptr;//TODO: separation SDL/ImageHandler => put in extension SDL
			::Image imageInfo;
			unsigned char* bytePtr;
			unsigned int idxMax;
			unsigned int request;
			struct{
				openLife::system::type::geometric::Point2D_32 point;
				unsigned int pixelIdx;
				openLife::system::type::geometric::Line2D_32 line;
				openLife::system::type::geometric::Circle2D_32 circle;
			}query;
	};
}


#endif //OPENLIFE_SYSTEM_OBJECT_PROCESS_CONVERT_IMAGE_H
