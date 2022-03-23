//
// Created by olivier on 05/12/2021.
//

#ifndef ONELIFE_GAME_DEBUG_H
#define ONELIFE_GAME_DEBUG_H

#include "data/value/context.h"

namespace openLife::debug
{
	class Console
	{
		public:
			static void write(const char* message, ...);
			static void write(openLife::debug::dataValue::Context context, const char* message, ...);
	};
}

#endif //ONELIFE_GAME_DEBUG_H
