//
// Created by olivier on 12/05/2022.
//

#include "NodeInquirer.h"

openLife::procedure::diagram::voronoi::fortuneAlgorithm::NodeInquirer::NodeInquirer()
{
	this->subject = nullptr;
}

openLife::procedure::diagram::voronoi::fortuneAlgorithm::NodeInquirer::~NodeInquirer(){}

void openLife::procedure::diagram::voronoi::fortuneAlgorithm::NodeInquirer::setSubject(beachline::BLNodePtr node)
{
	this->subject = node;
}

bool openLife::procedure::diagram::voronoi::fortuneAlgorithm::NodeInquirer::isRootNode()
{
	return (this->subject->parent.get()) ? false : true;//TODO search or non ambiguous condition
}

void openLife::procedure::diagram::voronoi::fortuneAlgorithm::NodeInquirer::printNodeInfo(const char* label)
{
	printf("\n\tbeachLine \"%s\"=> %p : {id:%i, coord:(%f, %f) indice:[%i-%i] parent[%i] child[left:%i, right:%i]}",
	   	(label ? label : ""),
		this->subject.get(),
		this->subject->get_id(),
		this->subject->points->at(this->subject->get_id()).x,
		this->subject->points->at(this->subject->get_id()).y,
		this->subject->indices.first,
		this->subject->indices.second,
	   	(this->subject->parent ? this->subject->parent->get_id() : 0),
		(this->subject->left ? this->subject->left->get_id() : 0),
		(this->subject->right ? this->subject->right->get_id() : 0));
}
