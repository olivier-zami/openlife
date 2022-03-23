//
// Created by olivier on 05/12/2021.
//

#ifndef OPENLIFE_SYSTEM_TRACE_H
#define OPENLIFE_SYSTEM_TRACE_H

#include <cstdarg>

namespace openLife::system
{
	class Console
	{
		public:
			static void writeLine(const char* message, va_list args);
			static void writeLine(const char* message, ...);

		protected:
			static unsigned int bufferSize;
	};
}

#endif //OPENLIFE_SYSTEM_TRACE_H
