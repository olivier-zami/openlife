//
// Created by olivier on 11/08/2022.
//

#include "Sdl2.h"

SDL_Renderer* openLife::extension::Sdl2::renderer = nullptr;

void openLife::extension::Sdl2::setRenderer(SDL_Renderer* renderer)
{
	openLife::extension::Sdl2::renderer = renderer;
}