//
// Created by olivier on 12/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATYPE_VERTEX_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATYPE_VERTEX_H

#include <memory>

#include "../Types/Point2D.h"
#include "HalfEdge.h"

namespace beachline
{
	class HalfEdge;
	typedef std::shared_ptr<HalfEdge> HalfEdgePtr;
}

namespace beachline
{
	class Vertex
	{
		public:
			Point2D point;
			HalfEdgePtr edge; // The edge points towards this vertex [-->o]

			Vertex(const Point2D &pos, HalfEdgePtr incident_edge = nullptr);

			inline double x() { return point.x; }
			inline double y() { return point.y; }
	};

	typedef std::shared_ptr<Vertex> VertexPtr;
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATYPE_VERTEX_H
