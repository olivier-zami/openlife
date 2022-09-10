//
// Created by olivier on 20/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_EVENTINQUIRER_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_EVENTINQUIRER_H

#include "../dataType/Event.h"

namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm
{
	class EventInquirer
	{
		public:
			EventInquirer();
			~EventInquirer();

			void setSubject(EventPtr event);
			void printInfo(const char* label = nullptr);

		private:
			EventPtr subject;
	};
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_EVENTINQUIRER_H
