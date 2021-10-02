//
// Created by olivier on 01/10/2021.
//

#include "log.h"

#include <cstdio>

void openLife::system::Log::notice(const char *message, ...)
{
	unsigned int bSize = 512;
	char buffer[bSize];
	va_list args;
	va_start (args, message);
	vsnprintf (buffer, bSize, message, args);
	printf("\n===>%s", buffer);
	va_end (args);
}

void openLife::system::Log::trace(const char *message, ...)
{
	unsigned int bSize = 512;
	char buffer[bSize];
	va_list args;
	va_start (args, message);
	vsnprintf (buffer, bSize, message, args);
	printf("\n===>%s", buffer);
	va_end (args);
}