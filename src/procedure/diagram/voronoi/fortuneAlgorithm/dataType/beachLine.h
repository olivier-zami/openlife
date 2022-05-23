//
// Created by olivier on 15/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATYPE_BEACHLINE_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATYPE_BEACHLINE_H

#include "../Types/Point2D.h"

namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm::dataType::beachLine
{
	typedef enum{
		UNDEFINED,
		ARC,
		EDGE
	}NodeType;

	typedef struct{
		unsigned int id;
		Point2D point;
	}Site;
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATYPE_BEACHLINE_H
