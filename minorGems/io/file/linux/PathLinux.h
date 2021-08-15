//
// Created by olivier on 14/08/2021.
//

#ifndef ONELIFETEST_PATHLINUX_H
#define ONELIFETEST_PATHLINUX_H

#if defined __linux__
	#define system system
#elif defined _WIN32
	#define system Linux
#endif

#include "minorGems/io/file/Path.h"

#if defined system
	#undef system
#endif

#endif //ONELIFETEST_PATHLINUX_H
