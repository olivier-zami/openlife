//
// Created by olivier on 09/10/2021.
//

#include "src/system/_base/_macro/init_system.h"

#if defined(SYSTEM)&&(SYSTEM==linux)

#include "../_base/init.h"
#include "../_base/log.h"

#define osLinux system

void openLife::osLinux::init()
{
	openLife::system::Log::notice("Starting Linux version of openLife ...");
}

#undef osLinux

#endif