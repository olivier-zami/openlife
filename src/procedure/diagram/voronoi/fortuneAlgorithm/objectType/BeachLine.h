//
// Created by olivier on 11/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_BEACHLINE_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_BEACHLINE_H

#include <vector>


#include "../dataType/Event.h"
#include "../dataType/HalfEdge.h"
#include "Node.hpp"
#include "NodeInquirer.h"

using namespace beachline;

namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm
{
	class BeachLine
	{
		public:
			BeachLine();
			~BeachLine();

			void readSite(EventPtr event, const std::vector<Point2D> &site);

		//
		beachline::BLNodePtr getRootNode();

		//private:

			std::pair<BLNodePtr, BLNodePtr> breakpoints(BLNodePtr leaf);

			BLNodePtr createNode(const std::vector<Point2D>* _points = nullptr);

			BLNodePtr createTree(int index, int index_behind, double *sweepline,
							const std::vector<Point2D> *points,
							std::vector<HalfEdgePtr> &edges);

			BLNodePtr createSimpleTree(int index, int index_behind, double *sweepline,
									  const std::vector<Point2D> *points,
									  std::vector<HalfEdgePtr> &edges);

			NodeInquirer* inquire(beachline::BLNodePtr node);
			bool isCurrentNodeRoot();

			int currentSiteId;
			NodeInquirer* nodeInquirer;
			double sweepLinePosition;
			beachline::BLNodePtr currentNode;
			beachline::BLNodePtr root;
	};
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_BEACHLINE_H
