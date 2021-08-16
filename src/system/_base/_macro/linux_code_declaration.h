//
// Created by olivier on 15/08/2021.
//

#if defined __linux__
	#define system system
#elif defined _WIN32
	#define system Linux
#endif

#include INCLUDED_FILE

#undef INCLUDED_FILE

#if defined system
	#undef system
#endif
