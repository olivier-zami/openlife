//
// Created by olivier on 11/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_EVENTQUEUE_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_EVENTQUEUE_H

#include <queue>
#include <vector>

#include "../../FortuneAlgorithm.h"
#include "../dataType/Event.h"
#include "../Types/Point2D.h"

namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm
{
	class EventQueue
	{
		public:
			EventQueue();
			~EventQueue();

			void addSiteEvent(unsigned int index, Point2D randomPoint);
			EventPtr getNextEvent();

			std::priority_queue<EventPtr, std::vector<EventPtr>, EventPtrComparator>* get();
			bool isEmpty();

		private:
			std::priority_queue<EventPtr, std::vector<EventPtr>, EventPtrComparator>* pq;
	};
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_EVENTQUEUE_H
