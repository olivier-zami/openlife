//
// Created by olivier on 11/08/2022.
//

#include "TextureHandler.h"

#include <cstdio>

openLife::extension::sdl2::handler::TextureHandler::TextureHandler()
{
	this->texture = nullptr;
}

openLife::extension::sdl2::handler::TextureHandler::~TextureHandler(){}


void openLife::extension::sdl2::handler::TextureHandler::handle(SDL_Texture* texture)
{
	this->texture = texture;
}

void openLife::extension::sdl2::handler::TextureHandler::drawCircle(openLife::dataType::geometric::Circle2D_32 circle)
{
	if(!openLife::extension::Sdl2::renderer);//TODO createRenderer or exception

	SDL_Renderer* renderer = openLife::extension::Sdl2::renderer;

	//int error = SDL_SetRenderTarget(renderer, this->texture);
	//if(error) printf("\nFail to use texture a target : %s", SDL_GetError());//TODO createRenderer or exception

	int offsetx, offsety, d;
	int status;

	offsetx = 0;
	offsety = circle.radius;
	d = circle.radius -1;
	status = 0;

	while (offsety >= offsetx)
	{
		status += SDL_RenderDrawPoint(renderer, circle.center.x + offsetx, circle.center.y + offsety);
		status += SDL_RenderDrawPoint(renderer, circle.center.x + offsety, circle.center.y + offsetx);
		status += SDL_RenderDrawPoint(renderer, circle.center.x - offsetx, circle.center.y + offsety);
		status += SDL_RenderDrawPoint(renderer, circle.center.x - offsety, circle.center.y + offsetx);
		status += SDL_RenderDrawPoint(renderer, circle.center.x + offsetx, circle.center.y - offsety);
		status += SDL_RenderDrawPoint(renderer, circle.center.x + offsety, circle.center.y - offsetx);
		status += SDL_RenderDrawPoint(renderer, circle.center.x - offsetx, circle.center.y - offsety);
		status += SDL_RenderDrawPoint(renderer, circle.center.x - offsety, circle.center.y - offsetx);

		if (status < 0) { status = -1; break; }

		if (d >= 2*offsetx) {
			d -= 2*offsetx + 1;
			offsetx +=1;
		}
		else if (d < 2 * ((signed)circle.radius - offsety))
		{
			d += 2 * offsety - 1;
			offsety -= 1;
		}
		else
		{
			d += 2 * (offsety - offsetx - 1);
			offsety -= 1;
			offsetx += 1;
		}
	}

	//error = SDL_SetRenderTarget(renderer, NULL);
	//if(error){printf("\nFail to use default a target : %s", SDL_GetError());}
}

void openLife::extension::sdl2::handler::TextureHandler::drawFillCircle(
		openLife::dataType::geometric::Circle2D_32 circle)
{
	if(!openLife::extension::Sdl2::renderer);//TODO createRenderer or exception

	SDL_Renderer* renderer = openLife::extension::Sdl2::renderer;

	int offsetx, offsety, d;
	int status;

	offsetx = 0;
	offsety = circle.radius;
	d = circle.radius -1;
	status = 0;

	while (offsety >= offsetx) {

		status += SDL_RenderDrawLine(renderer, circle.center.x - offsety, circle.center.y + offsetx,
									 circle.center.x + offsety, circle.center.y + offsetx);
		status += SDL_RenderDrawLine(renderer, circle.center.x - offsetx, circle.center.y + offsety,
									 circle.center.x + offsetx, circle.center.y + offsety);
		status += SDL_RenderDrawLine(renderer, circle.center.x - offsetx, circle.center.y - offsety,
									 circle.center.x + offsetx, circle.center.y - offsety);
		status += SDL_RenderDrawLine(renderer, circle.center.x - offsety, circle.center.y - offsetx,
									 circle.center.x + offsety, circle.center.y - offsetx);

		if (status < 0) {
			status = -1;
			break;
		}

		if (d >= 2*offsetx) {
			d -= 2*offsetx + 1;
			offsetx +=1;
		}
		else if (d < 2 * ((signed)circle.radius - offsety)) {
			d += 2 * offsety - 1;
			offsety -= 1;
		}
		else {
			d += 2 * (offsety - offsetx - 1);
			offsety -= 1;
			offsetx += 1;
		}
	}
}