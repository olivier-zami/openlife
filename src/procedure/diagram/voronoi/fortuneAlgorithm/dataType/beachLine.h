//
// Created by olivier on 15/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATATYPE_BEACHLINE_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATATYPE_BEACHLINE_H

#include <memory>

#include "../dataType/Edge.h"
#include "../dataType/EdgeEnd.h"
#include "../Types/Point2D.h"

namespace FA = openLife::procedure::diagram::voronoi::fortuneAlgorithm;

namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm::dataType::beachLine
{
	typedef enum{
		UNDEFINED,
		SITE,
		EDGE
	}NodeType;

	typedef struct{
		unsigned int id;
		Point2D point;
	}Site;

	typedef std::shared_ptr<Site> SitePtr;
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATYPE_BEACHLINE_H
