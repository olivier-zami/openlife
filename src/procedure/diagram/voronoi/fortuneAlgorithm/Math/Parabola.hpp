//
//  Parabola.hpp
//  FortuneAlgo
//
//  Created by Dmytro Kotsur on 20/04/2018.
//  Copyright © 2018 Dmytro Kotsur. All rights reserved.
//

#ifndef Parabola_hpp
#define Parabola_hpp

#include "../Types/Point2D.h"


/**
 
 Calculate number of intersection points between two parabolas with foci `f1` and `f2` and with given `directrix`
 
 */
int intersectionPointsNum(const Point2D &f1, const Point2D &f2, double directrix);



#endif /* Parabola_hpp */
