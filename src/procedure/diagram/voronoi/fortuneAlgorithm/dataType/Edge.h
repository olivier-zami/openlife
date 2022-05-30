//
// Created by olivier on 27/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATATYPE_EDGE_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATATYPE_EDGE_H

#include <memory>
#include "EdgeEnd.h"

namespace FA = openLife::procedure::diagram::voronoi::fortuneAlgorithm;

namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm::dataType
{
	typedef struct{
		unsigned int id;
		unsigned int idSite[2];
		FA::dataType::EdgeEnd* end[2];

		void setEdgeEnd(FA::dataType::EdgeEnd* edgeEnd)
		{
			if(!end[0])
			{
				end[0] = edgeEnd;
			}
			else if(!end[1])
			{
				if((edgeEnd->point.y<end[0]->point.y) || (edgeEnd->point.y==end[0]->point.y && edgeEnd->point.x<end[0]->point.x))
				{
					end[1] = end[0];
					end[0] = edgeEnd;
				}
				else
				{
					end[1] = edgeEnd;
				}
			}

		}
	}Edge;

	typedef std::shared_ptr<Edge> EdgePtr;
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATATYPE_EDGE_H
