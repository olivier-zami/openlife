//
// Created by olivier on 20/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_DEBUG_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_DEBUG_H

#include "dataType/Event.h"
#include "objectType/EventInquirer.h"
#include "objectType/inquirer/EdgeDebug.h"
#include "objectType/Node.hpp"
#include "objectType/NodeInquirer.h"

namespace FA = openLife::procedure::diagram::voronoi::fortuneAlgorithm;

namespace openLife::procedure::diagram::voronoi
{
	class Debug
	{
		public:
			static FA::EventInquirer* inquire(EventPtr event);
			static FA::inquirer::EdgeDebug* inquire(FA::dataType::Edge* edge);
			static FA::NodeInquirer* inquire(beachline::BLNodePtr node);

		private:
			static FA::inquirer::EdgeDebug* edgeDebugInquirer;
			static FA::EventInquirer* eventInquirer;
			static FA::NodeInquirer* nodeInquirer;
	};
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_DEBUG_H
