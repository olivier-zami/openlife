//
// Created by olivier on 20/05/2022.
//

#include "Debug.h"

namespace NS = openLife::procedure::diagram::voronoi;
namespace FA = openLife::procedure::diagram::voronoi::fortuneAlgorithm;

FA::EventInquirer* openLife::procedure::diagram::voronoi::Debug::eventInquirer = nullptr;
FA::inquirer::EdgeDebug* openLife::procedure::diagram::voronoi::Debug::edgeDebugInquirer = nullptr;
FA::NodeInquirer* openLife::procedure::diagram::voronoi::Debug::nodeInquirer = nullptr;

FA::EventInquirer* openLife::procedure::diagram::voronoi::Debug::inquire(EventPtr event)
{
	if(!NS::Debug::eventInquirer) NS::Debug::eventInquirer = new FA::EventInquirer();
	NS::Debug::eventInquirer->setSubject(event);
	return NS::Debug::eventInquirer;
}

FA::inquirer::EdgeDebug* openLife::procedure::diagram::voronoi::Debug::inquire(FA::dataType::Edge* edge)
{
	if(!NS::Debug::edgeDebugInquirer) NS::Debug::edgeDebugInquirer = new FA::inquirer::EdgeDebug();
	NS::Debug::edgeDebugInquirer->setSubject(edge);
	return NS::Debug::edgeDebugInquirer;
}

FA::NodeInquirer * openLife::procedure::diagram::voronoi::Debug::inquire(beachline::BLNodePtr node)
{
	if(!NS::Debug::nodeInquirer) NS::Debug::nodeInquirer = new FA::NodeInquirer();
	NS::Debug::nodeInquirer->setSubject(node);
	return NS::Debug::nodeInquirer;
}