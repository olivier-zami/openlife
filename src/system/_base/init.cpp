//
// Created by olivier on 09/10/2021.
//

#include "src/system/_base/_macro/init_system.h"

#if defined(SYSTEM)&&(SYSTEM==system)

#include "init.h"
#include "log.h"

void openLife::system::init()
{
	openLife::system::Log::notice("Starting agnostic version of openLife ...");
}

#endif