//
// Created by olivier on 12/05/2022.
//

#include "NodeInquirer.h"

openLife::procedure::diagram::voronoi::fortuneAlgorithm::NodeInquirer::NodeInquirer()
{
	this->sitePoint = nullptr;
	this->subject = nullptr;
	this->sweepLine = nullptr;
}

openLife::procedure::diagram::voronoi::fortuneAlgorithm::NodeInquirer::~NodeInquirer(){}

Point2D openLife::procedure::diagram::voronoi::fortuneAlgorithm::NodeInquirer::getPoint()
{
	return this->sitePoint->at(this->subject->get_id());
}

FA::dataType::beachLine::Site* openLife::procedure::diagram::voronoi::fortuneAlgorithm::NodeInquirer::getSite()
{
	/*
	std::shared_ptr<disk_node>* u_poi
			= static_cast< std::shared_ptr<disk_node>* >(RayCallback.m_collisionObject->getUserPointer());*/

	//FA::dataType::beachLine::SitePtr site = std::shared_ptr<FA::dataType::beachLine::Site>((FA::dataType::beachLine::Site *)this->subject->reference);
	//FA::dataType::beachLine::SitePtr site = std::make_shared<FA::dataType::beachLine::Site>();
	//FA::dataType::beachLine::SitePtr site = static_cast<FA::dataType::beachLine::SitePtr>(this->subject->reference);
	return ((FA::dataType::beachLine::Site*)this->subject->reference);
	//return site;
}

void openLife::procedure::diagram::voronoi::fortuneAlgorithm::NodeInquirer::setSitePointsReference(
		std::vector <Point2D> *sitePoint)
{
	this->sitePoint = sitePoint;
}

void openLife::procedure::diagram::voronoi::fortuneAlgorithm::NodeInquirer::setSubject(beachline::BLNodePtr node)
{
	this->subject = node;
}

void openLife::procedure::diagram::voronoi::fortuneAlgorithm::NodeInquirer::setSweepLineReference(double *sweepLine)
{
	this->sweepLine = sweepLine;
}

bool openLife::procedure::diagram::voronoi::fortuneAlgorithm::NodeInquirer::isRootNode()
{
	return (this->subject->parent.get()) ? false : true;//TODO search or non ambiguous condition
}

void openLife::procedure::diagram::voronoi::fortuneAlgorithm::NodeInquirer::printNodeInfo(const char* label)
{
	switch(this->subject->type)
	{
		case FA::dataType::beachLine::NodeType::SITE:
			this->printSiteNodeInfo(label);
			break;
		default:
			printf("%s %s point[%i](%.1f, %.1f) indices:[%i,%i]",
				   label ? label : "",
				   "UNDEFINED",
				   this->subject->get_id(),
				   -1.,
				   -1.,
				   this->subject->indices.first,
				   this->subject->indices.second);
			break;
	}
}

void openLife::procedure::diagram::voronoi::fortuneAlgorithm::NodeInquirer::printNodeTreeInfo(const char *label)
{
	printf("%s",
		   (label ? label : ""));
}

void openLife::procedure::diagram::voronoi::fortuneAlgorithm::NodeInquirer::printSiteNodeInfo(const char *label)
{
	printf("%s %s point[%i](%.1f, %.1f)",
		   label ? label : "",
		   "SITE",
		   this->subject->get_id(),
		   ((FA::dataType::beachLine::Site*)(this->subject->reference))->point.x,
		   ((FA::dataType::beachLine::Site*)(this->subject->reference))->point.y);
}