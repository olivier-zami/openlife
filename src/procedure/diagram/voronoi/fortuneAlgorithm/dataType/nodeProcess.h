//
// Created by olivier on 15/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATYPE_BEACHLINE_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATYPE_BEACHLINE_H

#include "Cell.h"

namespace FA = openLife::procedure::diagram::voronoi::fortuneAlgorithm;

namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm::dataType::nodeProcess
{
	typedef struct
	{
		FA::dataType::Cell site;
		bool inProgress;
	}Site;
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATYPE_BEACHLINE_H
