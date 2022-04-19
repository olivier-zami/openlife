//
// Created by olivier on 16/08/2021.
//

#ifndef ONELIFETEST_SYSTEM_HASH_MURMURHASH_H
#define ONELIFETEST_SYSTEM_HASH_MURMURHASH_H

//-----------------------------------------------------------------------------
// MurmurHash2 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.
//-----------------------------------------------------------------------------
// Platform-specific functions and macros

// Microsoft Visual Studio
#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;

// Other compilers
#else	// defined(_MSC_VER)
	#include <cstdint>

#endif // !defined(_MSC_VER)

// Microsoft Visual Studio
#if defined(_MSC_VER)
	#define BIG_CONSTANT(x) (x)

// Other compilers
#else	// defined(_MSC_VER)
	#define BIG_CONSTANT(x) (x##LLU)

#endif // !defined(_MSC_VER)
//-----------------------------------------------------------------------------

// detect 32- or 64-bit environment

// Check windows
#if _WIN32 || _WIN64
	#if _WIN64
		#define ENVIRONMENT64
	#else
		#define ENVIRONMENT32
	#endif
#endif

// Check GCC
#if __GNUC__
	#if __x86_64__ || __ppc64__
		#define ENVIRONMENT64
	#else
		#define ENVIRONMENT32
	#endif
#endif

// pick a version of the hash based on 32- or 64-bit environment
#ifdef ENVIRONMENT64
	#define MurmurHash MurmurHash64A
	#define TEST_ENV "nous sommes dans un env 64 bit"
#else
	#define MurmurHash MurmurHash64B
	#define TEST_ENV "nous sommes dans un env 32 bit"
#endif

#define MURMURHASH64_LINEARDB_SEED1 0xb9115a39

namespace openLife::system::hash
{
	uint64_t MurmurHash64A(const void* key, int len, uint64_t seed);
	uint64_t MurmurHash64B(const void* key, int len, uint64_t seed);
}

#endif //ONELIFETEST_SYSTEM_HASH_MURMURHASH_H
