//
// Created by olivier on 20/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_DEBUG_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_DEBUG_H

#include "dataType/Event.h"
#include "objectType/EventInquirer.h"

namespace FA = openLife::procedure::diagram::voronoi::fortuneAlgorithm;

namespace openLife::procedure::diagram::voronoi
{
	class Debug
	{
		public:
			static FA::EventInquirer* inquire(EventPtr event);

		private:
			static FA::EventInquirer* eventInquirer;
	};
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_DEBUG_H
