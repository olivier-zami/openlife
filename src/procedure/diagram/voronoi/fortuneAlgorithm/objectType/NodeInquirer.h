//
// Created by olivier on 12/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_NODEINQUIRER_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_NODEINQUIRER_H

#include <vector>
#include "../Types/Point2D.h"
#include "../dataType/beachLine.h"
#include "../dataType/Site.h"
#include "Node.hpp"

namespace FA = openLife::procedure::diagram::voronoi::fortuneAlgorithm;

namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm
{
	class NodeInquirer
	{
		public:
			NodeInquirer();
			~NodeInquirer();

			Point2D getPoint();
			FA::dataType::Site* getSite();
			void setSitePointsReference(std::vector<Point2D>* sitePoint);
			void setSubject(beachline::BLNodePtr node);
			void setSweepLineReference(double* sweepLine);

			bool isRootNode();
			void printNodeInfo(const char* label = nullptr);
			void printNodeTreeInfo(const char* label = nullptr);
			void printSiteNodeInfo(const char* label = nullptr);

		private:
			std::vector<Point2D>* sitePoint;
			beachline::BLNodePtr subject;
			double* sweepLine;
	};
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_NODEINQUIRER_H
