//
//  Parabola.cpp
//  FortuneAlgo
//
//  Created by Dmytro Kotsur on 20/04/2018.
//  Copyright © 2018 Dmytro Kotsur. All rights reserved.
//

#include "Parabola.hpp"

/**
 
 Calculate number of intersection points between two parabolas with foci `f1` and `f2` and with given `directrix`
 
 */
int intersectionPointsNum(const Point2D &f1, const Point2D &f2, double directrix) {
    if (fabs(f1.x - f2.x) < POINT_EPSILON && fabs(f1.y - f2.y) < POINT_EPSILON) {
        return -1;
    }
    if (fabs(f1.y - f2.y) < POINT_EPSILON)
        return 1;
    return 2;
}


/**
 Code for testing :)
 */

/*
int main(int argc, char *argv[]) {
    
    std::vector<Point2D> ips = findIntersectionPoints(Point2D(-4.0, 2.0), Point2D(1.0, 1.0), -10.0);
    
    for (Point2D p : ips) {
        std::cout << p << std::endl;
    }
    
    return 0;
}
 */

