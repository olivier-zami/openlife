//
// Created by olivier on 11/08/2022.
//

#ifndef __OPEN_LIFE__EXTENSION__SDL2__HANDLER__TEXTURE_HANDLER_H
#define __OPEN_LIFE__EXTENSION__SDL2__HANDLER__TEXTURE_HANDLER_H

#include "../../../dataType/geometric.h"
#include "../../Sdl2.h"

#include <SDL2/SDL.h>

namespace openLife::extension::sdl2::handler
{
	class TextureHandler :
		public openLife::extension::Sdl2
	{
		public:
			TextureHandler();
			~TextureHandler();

			void handle(SDL_Texture* texture);

			void drawCircle(openLife::dataType::geometric::Circle2D_32 circle);
			void drawFillCircle(openLife::dataType::geometric::Circle2D_32 circle);

		protected:
			SDL_Texture* texture;
	};
}



#endif //__OPEN_LIFE__EXTENSION__SDL2__HANDLER__TEXTURE_HANDLER_H
