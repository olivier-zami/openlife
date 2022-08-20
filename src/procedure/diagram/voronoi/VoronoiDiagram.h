//
// Created by olivier on 10/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_H

#include "fortuneAlgorithm/objectType/Node.hpp"
#include "fortuneAlgorithm/Datastruct/DCEL.hpp"
#include "fortuneAlgorithm/dataType/Edge.h"

namespace FA = openLife::procedure::diagram::voronoi::fortuneAlgorithm;

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
		std::vector<FA::dataType::EdgePtr>* edge;
		struct{
			std::vector<Point2D> point;
			std::vector<Point2D> point1;
		}debug;
	}VoronoiDiagram;
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_H
