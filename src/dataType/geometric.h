//
// Created by olivier on 02/09/2021.
//

#ifndef __OPEN_LIFE__DATATYPE__GEOMETRIC_H
#define __OPEN_LIFE__DATATYPE__GEOMETRIC_H

#include "../system/_base/type.h"

namespace openLife::dataType::geometric
{
	typedef openLife::system::type::Value2D_32 Point2D_32;
	typedef openLife::system::type::Value2D_U32 Point2D_U32;

	typedef struct{
		uint32_t radius;
		openLife::system::type::Value2D_32 center;
	}Circle2D_32;

	typedef struct{
		openLife::system::type::Value2D_32 point[2];
	}Line2D_32;

	typedef struct{
		float x;
		float y;
	}Point2D_F32;
}

#endif //__OPEN_LIFE__DATATYPE__GEOMETRIC_H
