//
// Created by olivier on 21/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_CELL_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_CELL_H

#include "../Types/Point2D.h"

namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm::dataType
{
	typedef struct{
		unsigned int id;
		Point2D point;
	}Cell;
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_CELL_H
