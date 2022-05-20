//
// Created by olivier on 10/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_H

#include "fortuneAlgorithm/objectType/Node.hpp"
#include "fortuneAlgorithm/Datastruct/DCEL.hpp"

namespace openLife::procedure::diagram::voronoi
{
	typedef struct{
		Point2D p1;
		Point2D p2;
	}Edge;

	typedef struct{
		struct{
			double width;
			double height;
		}dimension;
		std::vector<beachline::HalfEdgePtr> halfedges;
		std::vector<beachline::HalfEdgePtr> faces;
		std::vector<beachline::VertexPtr> vertices;
	}VoronoiDiagram;
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_H
