//
// Created by olivier on 09/10/2021.
//

#include "src/system/_base/_macro/init_system.h"

#if defined(SYSTEM)&&(SYSTEM==mac)

#include "../_base/init.h"
#include "../_base/log.h"

#define osMac system

void openLife::osMac::init()
{
	openLife::system::Log::notice("Starting Mac version of openLife ...");
}

#undef osMac

#endif