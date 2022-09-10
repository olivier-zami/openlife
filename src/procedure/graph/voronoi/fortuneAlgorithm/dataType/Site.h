//
// Created by olivier on 31/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATATYPE_SITE_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATATYPE_SITE_H

namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm::dataType
{
	typedef struct{
		unsigned int id;
		Point2D point;
	}Site;

	typedef std::shared_ptr<Site> SitePtr;
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATATYPE_SITE_H
