//
// Created by olivier on 05/12/2021.
//

#include "trace.h"

#include <cstdio>
#include <cstring>

unsigned int openLife::system::Trace::bufferSize = 512;

void openLife::system::Trace::writeLine(const char* message, va_list args)
{
	char buffer[openLife::system::Trace::bufferSize];
	memset(buffer, 0, sizeof(buffer));
	vsnprintf (buffer, openLife::system::Trace::bufferSize, message, args);
	va_end(args);
	printf("\n%s", buffer);
}

void openLife::system::Trace::writeLine(const char* message, ...)
{
	va_list args;
	va_start (args, message);
	openLife::system::Trace::writeLine(message, args);
}