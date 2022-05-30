//
// Created by olivier on 27/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATATYPE_EDGEEND_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATATYPE_EDGEEND_H

#include "../Types/Point2D.h"

namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm::dataType
{
	typedef struct{
		unsigned int id;
		Point2D point;
	}EdgeEnd;

	typedef std::shared_ptr<EdgeEnd> EdgeEndPtr;
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATATYPE_EDGE_H
