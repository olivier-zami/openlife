//
// Created by olivier on 11/08/2022.
//

#ifndef __OPEN_LIFE__EXTENSION__SDL2_H
#define __OPEN_LIFE__EXTENSION__SDL2_H

#include <SDL2/SDL.h>

namespace openLife::extension
{
	class Sdl2
	{
		public:
			static void setRenderer(SDL_Renderer* renderer);

		protected:
			static SDL_Renderer* renderer;
	};
}


#endif //__OPEN_LIFE__EXTENSION__SDL2_H
