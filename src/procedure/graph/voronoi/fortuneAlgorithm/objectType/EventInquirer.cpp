//
// Created by olivier on 20/05/2022.
//

#include "EventInquirer.h"

namespace FA = openLife::procedure::diagram::voronoi::fortuneAlgorithm;

FA::EventInquirer::EventInquirer() {}
FA::EventInquirer::~EventInquirer() {}

void FA::EventInquirer::setSubject(EventPtr event)
{
	this->subject = event;
}

void FA::EventInquirer::printInfo(const char *label)
{
	printf("%s",
		   (label ? label : ""));
}
