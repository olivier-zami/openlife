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
	this->idxMax = (height * width) - 1;
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

openLife::system::object::process::handler::Image* openLife::system::object::process::handler::Image::select(openLife::system::type::geometric::Point2D_32 point)
{
	if(point.x>=this->imageInfo.width || point.y>=this->imageInfo.height) this->request = 0;//TODO: system level => throw Exception
	else
	{
		this->request = 1;
		this->query.pixelIdx = point.x + (this->imageInfo.width * point.y);
	}
	return this;
}

openLife::system::object::process::handler::Image *openLife::system::object::process::handler::Image::select(openLife::system::type::geometric::Circle2D_32 circle)
{
	this->request = 3;
	this->query.circle = circle;
	return this;
}

openLife::system::object::process::handler::Image *openLife::system::object::process::handler::Image::select(openLife::system::type::geometric::Line2D_32 line)
{
	this->request = 2;//TODO: class const for line request...
	this->query.line = line;
	return this;
}

void openLife::system::object::process::handler::Image::setPixel(ColorRGB color)
{
	unsigned int idx = this->query.pixelIdx * this->imageInfo.bytesPerPixel;
	switch(this->request)
	{
		case 1:
			this->drawPixel(idx, color);
			break;
		case 2:
			this->drawLine(this->query.line, color);
			break;
		case 3:
			this->drawCircle(this->query.circle, color);
			this->request = 0;
			break;
		default:
			break;
	}
}

ColorRGB openLife::system::object::process::handler::Image::getPixel()
{
	unsigned int idx = this->query.pixelIdx * this->imageInfo.bytesPerPixel;
	ColorRGB color;
	color.b = this->bytePtr[idx];
	color.g = this->bytePtr[idx+1];
	color.r = this->bytePtr[idx+2];
	return color;
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

void openLife::system::object::process::handler::Image::drawLine(openLife::system::type::geometric::Line2D_32  line, ColorRGB color)//Bresenham line
{
	int dx = line.point[1].x - line.point[0].x;
	int dy = line.point[1].y - line.point[0].y;

	//!Angle: [0] PI
	if(!dx && dy>0)
	{
		for(int y=line.point[0].y; y<line.point[1].y; y++)
		{
			this->select((openLife::system::type::geometric::Point2D_32){line.point[0].x, y})->setPixel(color);
		}
	}

	//!Angle: ]0, PI/4[
	if(dx>0 && dy>0 && dx<dy)
	{
		// vecteur oblique proche de la verticale, dans le 2d octant
		int e = dy;
		dy = e * 2 ;
		dx = dx * 2 ;
		// e est positif
		int x = line.point[0].x;
		for(int y=line.point[0].y; y<line.point[1].y; y++)// déplacements verticaux
		{
			this->select((openLife::system::type::geometric::Point2D_32){x, y})->setPixel(color);
			e -= dx;
			if(e < 0){x++; e += dy;}// déplacement diagonal
		}
	}

	//!Angle: [PI/4, PI/2[
	if(dx>0 && dy>0 && dx>=dy) //vecteur oblique dans le 1er quadran
	{
		int e = dx;
		dx = e * 2; // e est positif
		dy = dy * 2;
		int y = line.point[0].y;
		for(int x=line.point[0].x; x<line.point[1].x; x++)
		{
			this->select((openLife::system::type::geometric::Point2D_32){x, y})->setPixel(color);
			e -= dy;
			if(e<0){y++; e+= dx;}// déplacement diagonal
		}
	}

	//!Angle: [PI/2]
	if(!dy && dx>0)
	{
		for(int x=line.point[0].x; x<line.point[1].x; x++)
		{
			this->select((openLife::system::type::geometric::Point2D_32){x, line.point[0].y})->setPixel(color);
		}
	}

	//!Angle ]PI/2, 3PI/4]
	if(dx>0 && dy<0 && dx>=-dy)// vecteur oblique dans le 4e cadran
	{
		// vecteur diagonal ou oblique proche de l’horizontale, dans le 8e octant
		int e = dx;
		dx = e * 2 ;
		dy = dy * 2 ;
		// e est positif
		int y = line.point[0].y;
		for(int x=line.point[0].x; x<line.point[1].x; x++)// déplacements horizontaux
		{
			this->select((openLife::system::type::geometric::Point2D_32){x, y})->setPixel(color);
			e += dy;
			if(e<0){y--; e+=dx;}// déplacement diagonal
		}
	}

	//!Angle: ]3PI/4, PI[
	if(dx>0 && dy<0 && dx<-dy)// vecteur oblique proche de la verticale, dans le 7e octant
	{
		int e = dy;
		dy = e * 2 ;
		dx = dx * 2 ;  // e est négatif
		int x = line.point[0].x;
		for(int y=line.point[0].y; y>line.point[1].y; y--)// déplacements verticaux
		{
			this->select((openLife::system::type::geometric::Point2D_32){x, y})->setPixel(color);
			e += dx;
			if(e>0){x++;e+=dy;}// déplacement diagonal
		}
	}

	//!Angle: [PI]
	if(!dx && dy<0)
	{
		for(int y=line.point[0].y; y>line.point[1].y; y--)
		{
			this->select((openLife::system::type::geometric::Point2D_32){line.point[0].x, y})->setPixel(color);
		}
	}

	//!Angle: ]PI, 5PI/4[
	if(dx<0 && dy<0 && dx>dy)
	{
		// vecteur oblique proche de la verticale, dans le 6e octant
		int e = dy;
		dy = e * 2;
		dx = dx * 2;
		// e est négatif
		int x = line.point[0].x;
		for(int y = line.point[0].y; y>line.point[1].y; y--)// déplacements verticaux
		{
			this->select((openLife::system::type::geometric::Point2D_32){x, y})->setPixel(color);
			e -= dx;
			if(e >= 0){x--; e+=dy;}
		}
	}

	//!Angle: [5PI/4, 3PI/2[
	if(dx<0 && dy<0 && dx<=dy)// vecteur oblique dans le 3e cadran
	{
		// vecteur diagonal ou oblique proche de l’horizontale, dans le 5e octant
		int e = dx;
		dx = e * 2;
		dy = dy * 2;
		// e est négatif
		int y = line.point[0].y;
		for(int x = line.point[0].x; x>line.point[1].x; x--)// déplacements horizontaux
		{
			this->select((openLife::system::type::geometric::Point2D_32){x, y})->setPixel(color);
			e -= dy;
			if(e >=0){y--; e+= dx;}// déplacement diagonal
		}
	}

	//!Angle: [3PI/2]
	if(!dy && dx<0)// dy = 0 (et dx < 0)
	{
		for(int x=line.point[0].x; x>line.point[1].x; x--)// vecteur horizontal vers la gauche
		{
			this->select((openLife::system::type::geometric::Point2D_32){x, line.point[0].y})->setPixel(color);
		}
	}

	//!Angle: ]3PI/2, 7PI/4]
	if(dx<0 && dy>0 && -dx>=dy)// vecteur oblique dans le 2d quadran
	{
		// vecteur diagonal ou oblique proche de l’horizontale, dans le 4e octant
		int e = dx;
		dx = e * 2;
		dy = dy * 2;
		// e est négatif
		int y = line.point[0].y;
		for(int x=line.point[0].x; x>line.point[1].x; x--)// déplacements horizontaux
		{
			this->select((openLife::system::type::geometric::Point2D_32){x, y})->setPixel(color);
			e += dy;
			if(e >= 0){y++; e+=dx;} // déplacement diagonal
		}
	}

	//!Angle: ]7PI/4, 2PI[
	if(dx<0 && dy>0 && -dx<dy)
	{
		// vecteur oblique proche de la verticale, dans le 3e octant
		int e = dy;
		dy = e * 2 ;
		dx = dx * 2 ;
		// e est positif
		int x = line.point[0].x;
		for(int y=line.point[0].y; y<line.point[1].y; y++)// déplacements verticaux
		{
			this->select((openLife::system::type::geometric::Point2D_32){x, y})->setPixel(color);
			e += dx;
			if(e<=0){x--; e+=dy;}// déplacement diagonal
		}
	}
	// le pixel final (x2, y2) n’est pas tracé.
}

void openLife::system::object::process::handler::Image::drawPixel(unsigned int idx, ColorRGB color)
{
	this->bytePtr[idx] = (unsigned char)color.b;
	this->bytePtr[idx+1] = (unsigned char)color.g;
	this->bytePtr[idx+2] = (unsigned char)color.r;
}

void openLife::system::object::process::handler::Image::drawCircle(openLife::system::type::geometric::Circle2D_32 circle, ColorRGB color)
{
	int x = 0;
	int y = circle.radius;
	int d = 3 - 2 * circle.radius;

	this->select((openLife::system::type::geometric::Point2D_32){circle.center.x+x, circle.center.y+y})->setPixel(color);
	this->select((openLife::system::type::geometric::Point2D_32){circle.center.x+x, circle.center.y+y})->setPixel(color);
	this->select((openLife::system::type::geometric::Point2D_32){circle.center.x-x, circle.center.y+y})->setPixel(color);
	this->select((openLife::system::type::geometric::Point2D_32){circle.center.x+x, circle.center.y-y})->setPixel(color);
	this->select((openLife::system::type::geometric::Point2D_32){circle.center.x-x, circle.center.y-y})->setPixel(color);
	this->select((openLife::system::type::geometric::Point2D_32){circle.center.x+y, circle.center.y+x})->setPixel(color);
	this->select((openLife::system::type::geometric::Point2D_32){circle.center.x-y, circle.center.y+x})->setPixel(color);
	this->select((openLife::system::type::geometric::Point2D_32){circle.center.x+y, circle.center.y-x})->setPixel(color);
	this->select((openLife::system::type::geometric::Point2D_32){circle.center.x-y, circle.center.y-x})->setPixel(color);

	while (y >= x)
	{
		x++;
		if (d > 0)
		{
			y--;
			d = d + 4 * (x - y) + 10;
		}
		else
			d = d + 4 * x + 6;

		this->select((openLife::system::type::geometric::Point2D_32){circle.center.x+x, circle.center.y+y})->setPixel(color);
		this->select((openLife::system::type::geometric::Point2D_32){circle.center.x+x, circle.center.y+y})->setPixel(color);
		this->select((openLife::system::type::geometric::Point2D_32){circle.center.x-x, circle.center.y+y})->setPixel(color);
		this->select((openLife::system::type::geometric::Point2D_32){circle.center.x+x, circle.center.y-y})->setPixel(color);
		this->select((openLife::system::type::geometric::Point2D_32){circle.center.x-x, circle.center.y-y})->setPixel(color);
		this->select((openLife::system::type::geometric::Point2D_32){circle.center.x+y, circle.center.y+x})->setPixel(color);
		this->select((openLife::system::type::geometric::Point2D_32){circle.center.x-y, circle.center.y+x})->setPixel(color);
		this->select((openLife::system::type::geometric::Point2D_32){circle.center.x+y, circle.center.y-x})->setPixel(color);
		this->select((openLife::system::type::geometric::Point2D_32){circle.center.x-y, circle.center.y-x})->setPixel(color);
	}
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
