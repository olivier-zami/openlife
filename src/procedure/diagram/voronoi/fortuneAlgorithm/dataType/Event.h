//
// Created by olivier on 11/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATYPE_EVENT_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATYPE_EVENT_H

#include "../objectType/Node.hpp"
#include "../Types/Point2D.h"

namespace beachline
{
	typedef enum {
		NONE,
		SITE,
		CIRCLE
	}EventType;
}

namespace beachline
{
	typedef struct{
		EventType type;

		//!data
		std::pair<int, int> indices;
	}EventNode;
}

class Event;

struct Event {

	enum { SITE = 0, CIRCLE = 1, SKIP = 2, };


	int type;
	Point2D point;

	/*
	 Site event attributes:
	 */
	int index;

	/*
	 Circle event attributes:
	 */
	Point2D center;
	beachline::BLNodePtr arc;


	Event(
			int _index = -1,
			int _type = Event::SKIP,
			const Point2D &_point = Point2D(0.0, 0.0)
					) : index(_index), type(_type), point(_point), arc(nullptr)
	{}

};

typedef std::shared_ptr<Event> EventPtr;

struct EventPtrComparator {
	Point2DComparator point_cmp;
	bool operator()(const EventPtr &e1, const EventPtr &e2) {
		return point_cmp(e1->point, e2->point);
	}
};

EventPtr checkCircleEvent(
		beachline::BLNodePtr n1,
		beachline::BLNodePtr n2,
		beachline::BLNodePtr n3,
		const std::vector<Point2D> &points,
		double sweepline);

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATYPE_EVENT_H
