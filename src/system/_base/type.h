//
// Created by olivier on 23/08/2021.
//

#ifndef OPENLIFE_SYSTEM_TYPE_H
#define OPENLIFE_SYSTEM_TYPE_H

namespace openLife::system::type
{
	typedef struct {
		long int x;
		long int y;
	}Coord2D;

	typedef struct{
		unsigned long int width;
		unsigned long int height;
	}Dimension2D;
}

#endif //OPENLIFE_SYSTEM_TYPE_H
