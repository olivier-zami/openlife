//
// Created by olivier on 14/08/2021.
//

#ifndef ONELIFETEST_PATHWIN32_H
#define ONELIFETEST_PATHWIN32_H

#if defined __linux__
	#define system Win32
#elif defined _WIN32
	#define system system
#endif

#include "minorGems/io/file/Path.h"

#if defined system
	#undef system
#endif

#endif //ONELIFETEST_PATHWIN32_H
