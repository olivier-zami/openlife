//
// Created by olivier on 22/09/2021.
//

#ifndef OPENLIFE_CLIENT_GAME_H
#define OPENLIFE_CLIENT_GAME_H

#include "src/client/component/bank/sprite.h"

namespace client
{
	class Game
	{
		public:
			Game();
			~Game();

			client::component::bank::Sprite* getSprites();

		private:
			client::component::bank::Sprite* spriteBank;
	};
}

#endif //OPENLIFE_CLIENT_GAME_H
