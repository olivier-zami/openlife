//
// Created by olivier on 27/05/2022.
//

#include "EdgeDebug.h"

#include <cstring>

namespace FA = openLife::procedure::diagram::voronoi::fortuneAlgorithm;

FA::inquirer::EdgeDebug::EdgeDebug()
{
	this->subject = nullptr;
}

FA::inquirer::EdgeDebug::~EdgeDebug() {}

void FA::inquirer::EdgeDebug::setSubject(FA::dataType::Edge* edge)
{
	this->subject = edge;
}

void FA::inquirer::EdgeDebug::printInfo(const char *label)
{
	char coordPoint1[64]; memset(coordPoint1, 0, sizeof(coordPoint1));//TODO dynamic allocation for coordPoint1, coordPoint2
	char coordPoint2[64]; memset(coordPoint2, 0, sizeof(coordPoint2));

	if(!this->subject->end[0]) sprintf(coordPoint1, "-");
	else
	{
		sprintf(coordPoint1, "[%i](%.1f, %.1f)",
		  	this->subject->end[0]->id,
		  	this->subject->end[0]->point.x,
		  	this->subject->end[0]->point.y);
	}

	if(!this->subject->end[1]) sprintf(coordPoint2, "-");
	else
	{
		sprintf(coordPoint2, "[%i](%.1f, %.1f)",
				this->subject->end[1]->id,
				this->subject->end[1]->point.x,
				this->subject->end[1]->point.y);
	}

	printf("%s [%i] site[%i, %i] (%s, %s)",
		   label ? label : "",
		   this->subject->id,
		   this->subject->idSite[0],
		   this->subject->idSite[1],
		   coordPoint1,
		   coordPoint2);
}
