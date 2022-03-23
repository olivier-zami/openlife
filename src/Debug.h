//
// Created by olivier on 22/03/2022.
//

#ifndef OPENLIFE_DEBUG_H
#define OPENLIFE_DEBUG_H

#include <cstdarg>
#include "debug/data/value/context.h"
#include "system/Console.h"

namespace openLife
{
	class Debug :
			public openLife::system::Console
	{
		public:
			static void setContext(openLife::debug::dataValue::Context context);
			static void write(const char* message, ...);
			//TODO: writeContextualMessage(); if context => printf(...);

		protected:
			static unsigned int bufferSize;

		private:
			static openLife::debug::dataValue::Context context;
	};
}

#endif //OPENLIFE_DEBUG_H
