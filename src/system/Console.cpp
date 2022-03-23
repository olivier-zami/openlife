//
// Created by olivier on 05/12/2021.
//

#include "Console.h"

#include <cstdio>
#include <cstring>

unsigned int openLife::system::Console::bufferSize = 512;

void openLife::system::Console::writeLine(const char* message, va_list args)
{
	char buffer[openLife::system::Console::bufferSize];
	memset(buffer, 0, sizeof(buffer));
	vsnprintf (buffer, openLife::system::Console::bufferSize, message, args);
	va_end(args);
	printf("\n%s", buffer);
}

void openLife::system::Console::writeLine(const char* message, ...)
{
	va_list args;
	va_start (args, message);
	openLife::system::Console::writeLine(message, args);
}