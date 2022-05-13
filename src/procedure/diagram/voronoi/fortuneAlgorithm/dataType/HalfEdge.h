//
// Created by olivier on 12/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATYPE_EDGE_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATYPE_EDGE_H


#include "Vertex.h"

namespace beachline
{
	class Vertex;
	typedef std::shared_ptr<Vertex> VertexPtr;
}

namespace beachline
{
	class HalfEdge;
	typedef std::shared_ptr<HalfEdge> HalfEdgePtr;

	class HalfEdge {
	public:

		int l_index, r_index;

		VertexPtr vertex;
		HalfEdgePtr twin;
		HalfEdgePtr next;
		HalfEdgePtr prev;

		HalfEdge(int _l_index, int _r_index, VertexPtr _vertex = nullptr);

		inline VertexPtr vertex0() { return vertex; }
		inline VertexPtr vertex1() { return twin->vertex; }
		inline bool is_finite() {
			return vertex != nullptr && twin->vertex != nullptr;
		}

		// Iterators around vertex
		HalfEdgePtr vertexNextCCW();
		HalfEdgePtr vertexNextCW();
	};

	std::pair<HalfEdgePtr, HalfEdgePtr> make_twins(int left_index, int right_index);
	std::pair<HalfEdgePtr, HalfEdgePtr> make_twins(const std::pair<int,int> &indices);
	void connect_halfedges(HalfEdgePtr p1, HalfEdgePtr p2);

}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATYPE_EDGE_H
