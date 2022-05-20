//
// Created by olivier on 20/05/2022.
//

#include "Debug.h"

namespace NS = openLife::procedure::diagram::voronoi;
namespace FA = openLife::procedure::diagram::voronoi::fortuneAlgorithm;

FA::EventInquirer* openLife::procedure::diagram::voronoi::Debug::eventInquirer = nullptr;

FA::EventInquirer* openLife::procedure::diagram::voronoi::Debug::inquire(EventPtr event)
{
	if(!NS::Debug::eventInquirer) NS::Debug::eventInquirer = new FA::EventInquirer();
	NS::Debug::eventInquirer->setSubject(event);
	return NS::Debug::eventInquirer;
}
