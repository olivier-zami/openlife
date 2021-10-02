//
// Created by olivier on 01/10/2021.
//

#ifndef OPENLIFE_LOG_H
#define OPENLIFE_LOG_H

#include <cstdarg>

namespace openLife::system
{
	class Log
	{
		public:
			static void notice(const char* message, ...);
			static void trace(const char* message, ...);
	};

}

#endif //OPENLIFE_LOG_H
