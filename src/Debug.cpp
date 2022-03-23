//
// Created by olivier on 22/03/2022.
//

#include "Debug.h"

#include <cstring>
#include <cstdio>

openLife::debug::dataValue::Context openLife::Debug::context;
unsigned int openLife::Debug::bufferSize = 512;

void openLife::Debug::setContext(openLife::debug::dataValue::Context context)
{
	openLife::Debug::context = context;
}

void openLife::Debug::write(const char* message, ...)
{
	va_list args;
	va_start (args, message);

	char buffer[openLife::Debug::bufferSize];
	memset(buffer, 0, sizeof(buffer));
	char preMessage[] = "###debug### ";
	strcpy(buffer, preMessage);
	vsnprintf (buffer+strlen(preMessage), openLife::Debug::bufferSize, message, args);
	va_end(args);

	printf("\n%s", buffer);
}
