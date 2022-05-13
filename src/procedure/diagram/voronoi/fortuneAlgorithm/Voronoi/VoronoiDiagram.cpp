//
//  VoronoiDiagram.cpp
//  FortuneAlgo
//
//  Created by Dmytro Kotsur on 06/05/2018.
//  Copyright © 2018 Dmytro Kotsur. All rights reserved.
//

#include "VoronoiDiagram.hpp"
#include "../dataType/Event.h"
#include "../objectType/Node.hpp"
#include "../Math/Parabola.hpp"
#include "../Math/Circle.hpp"
#include "../Datastruct/DCEL.hpp"

#include <queue>

#define BREAKPOINTS_EPSILON 1.0e-5
#define _DEBUG_

namespace bl = beachline;

EventPtr checkCircleEvent(bl::BLNodePtr n1, bl::BLNodePtr n2, bl::BLNodePtr n3,
                          const std::vector<Point2D> &points, double sweepline) {
    
    if (n1 == nullptr || n2 == nullptr || n3 == nullptr)
        return nullptr;
    
    Point2D p1 = points[n1->get_id()];
    Point2D p2 = points[n2->get_id()];
    Point2D p3 = points[n3->get_id()];
    Point2D center, bottom;
    
    if (p2.y > p1.y && p2.y > p3.y)
        return nullptr;
    
    if (!findCircleCenter(p1, p2, p3, center))
        return nullptr;
    
    bottom = center;
    bottom.y += (center - p2).norm();
    
    // check circle event
    if (fabs(bottom.y - sweepline) < POINT_EPSILON || sweepline < bottom.y) {
        // create a circle event structure
        EventPtr e = std::make_shared<Event>(-1, Event::CIRCLE, bottom);
        // initialize attributes
        e->center = center;
        e->arc = n2;
        // add reference in the corresponding node
        n2->circle_event = e;
        return e;
    }
    
    return nullptr;
}

