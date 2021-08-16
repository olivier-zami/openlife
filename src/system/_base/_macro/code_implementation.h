//
// Created by olivier on 15/08/2021.
//
#if !defined(Win32)
	#if defined(_WIN32)
		#define Win32 system
	#else
		#define Win32 Win32
	#endif
#endif

#if !defined(Linux)
	#if defined(__linux__)
		#define Linux system
	#else
		#define Linux Linux
	#endif
#endif
