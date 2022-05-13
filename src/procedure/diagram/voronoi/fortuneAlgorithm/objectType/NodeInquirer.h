//
// Created by olivier on 12/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_NODEINQUIRER_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_NODEINQUIRER_H

#include "Node.hpp"

namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm
{
	class NodeInquirer
	{
		public:
			NodeInquirer();
			~NodeInquirer();

			void setSubject(beachline::BLNodePtr node);

			bool isRootNode();
			void printNodeInfo(const char* label = nullptr);

		private:
			beachline::BLNodePtr subject;
	};
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_NODEINQUIRER_H
