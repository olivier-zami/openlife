//
// Created by olivier on 05/12/2021.
//

#include "Console.h"

#include <cstdarg>
#include <cstring>
#include <cstdio>
#include "../system/Console.h"

void openLife::debug::Console::write(const char* message, ...)
{
	va_list args;
	va_start (args, message);
	char buffer[512];
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%s", message);
	//TODO: write if context allow openLife::debug::dataValue::Context::NONE
	openLife::system::Console::writeLine(buffer, args);
}

void openLife::debug::Console::write(openLife::debug::dataValue::Context context, const char *message, ...)
{
	va_list args;
	va_start (args, message);
	char buffer[512];
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%s", message);
	//TODO: write if context allow context
	openLife::system::Console::writeLine(buffer, args);
}