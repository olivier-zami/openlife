//
// Created by olivier on 22/09/2021.
//

#ifndef OPENLIFE_SPRITE_H
#define OPENLIFE_SPRITE_H

#include "src/client/component/bank/sprite/ground.h"

namespace client::component::bank
{
	class Sprite
	{
		public:
			Sprite();
			~Sprite();

			client::component::bank::sprite::Ground* getGrounds();

		private:
			client::component::bank::sprite::Ground* groundBank;
	};
}

#endif //OPENLIFE_SPRITE_H
