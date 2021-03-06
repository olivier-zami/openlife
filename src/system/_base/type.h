//
// Created by olivier on 23/08/2021.
//

#ifndef OPENLIFE_SYSTEM_TYPE_H
#define OPENLIFE_SYSTEM_TYPE_H

#include <cstdint>

namespace openLife::system::type
{
	typedef struct{
		unsigned long int width;
		unsigned long int height;
	}Dimension2D;

	/*
	typedef struct{
		int32_t x;
		int32_t y;
	}Point2D_32;
*/
	typedef struct{
		uint32_t width;
		uint32_t height;
	}Size2D_32;

	typedef struct{
		int32_t x;
		int32_t y;
	}Value2D_32;

	typedef struct
	{
		uint32_t x;
		uint32_t y;
	}Value2D_U32;
}

#endif //OPENLIFE_SYSTEM_TYPE_H
