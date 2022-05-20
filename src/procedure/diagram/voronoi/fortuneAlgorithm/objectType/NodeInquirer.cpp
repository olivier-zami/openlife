//
// Created by olivier on 12/05/2022.
//

#include "NodeInquirer.h"

#include "../Math/Parabola.hpp"

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
	printf("%s [%i](%.1f, %.1f){indice:[%i-%i] parent:[%i] child: [left:%i, right:%i]}",
	   	(label ? label : ""),
		this->subject->get_id(),
	   	(this->sitePoint->at(this->subject->get_id()).x),
	   	(this->sitePoint->at(this->subject->get_id()).y),
		this->subject->indices.first,
		this->subject->indices.second,
	   	(this->subject->parent ? this->subject->parent->get_id() : 0),
		(this->subject->left ? this->subject->left->get_id() : 0),
		(this->subject->right ? this->subject->right->get_id() : 0));
}
