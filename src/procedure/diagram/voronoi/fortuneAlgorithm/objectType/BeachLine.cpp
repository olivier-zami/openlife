//
// Created by olivier on 11/05/2022.
//

#include "BeachLine.h"

#include <memory>
#include <utility>

#include "Node.hpp"

using namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm;

/**
 * @note// create a beachline tree
 */
openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::BeachLine()
{
	this->root = nullptr;
	this->currentSiteId = -1;
	this->nodeInquirer = new openLife::procedure::diagram::voronoi::fortuneAlgorithm::NodeInquirer();
	this->sweepLinePosition = 0L;
}

openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::~BeachLine() {}

void openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::readSite(EventPtr event, const std::vector<Point2D> &site)
{
	printf("\n\tread sitePoint: id=%i coord(%f, %f)", event->index, event->point.x, event->point.y);
	this->currentSiteId = event->index;//!legacy : point_i
	this->sweepLinePosition = event->point.y;

	if(!this->root)
	{
		//!init empty beach line tree
		this->root = this->createNode(&site);
		this->currentNode = this->root;
		this->inquire(this->root)->printNodeInfo("createRootNode");
	}
	else
	{
		this->currentNode = this->createNode(&site);
		this->inquire(this->currentNode)->printNodeInfo("createCurrentNode");
	}
}

beachline::BLNodePtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::getRootNode()
{
	return this->root;
}

/**
 *
 * @note Returns breakpoints for a given arc
 */
std::pair<BLNodePtr, BLNodePtr> openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::breakpoints(BLNodePtr leaf)
{
	this->inquire(leaf)->printNodeInfo("check for bp");
	if (leaf == nullptr || leaf->next == nullptr || leaf->prev == nullptr)
		return std::make_pair<BLNodePtr>(nullptr, nullptr);

	BLNodePtr parent = leaf->parent, gparent = leaf->parent;
	std::pair<int,int> bp1(leaf->prev->get_id(), leaf->get_id()); // left breakpoint
	std::pair<int,int> bp2(leaf->get_id(), leaf->next->get_id()); // right breakpoint
	std::pair<int,int> other_bp;

	bool left_is_missing = true;

	if (parent->has_indices(bp1)) {
		other_bp = bp2;
		left_is_missing = false;
	} else if (parent->has_indices(bp2)) {
		other_bp = bp1;
		left_is_missing = true;
	}

	// Go up and rebalance the whole tree
	while (gparent != nullptr) {
		if (gparent->has_indices(other_bp)) {
			break;
		}
		gparent = gparent->parent;
	}

	if (left_is_missing) {
		return std::make_pair(gparent, parent);
	} else {
		return std::make_pair(parent, gparent);
	}

//        // BUG doesn't take into account gparent WRONG!!!
//        if (parent->parent != nullptr) {
//            if (parent->parent->left == parent) {
//                return std::make_pair(parent, gparent);
//            } else {
//                return std::make_pair(gparent, parent);
//            }
//        }
//
//        return std::make_pair(parent, gparent);
}

BLNodePtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::createNode(const std::vector<Point2D>* _points)
{
	BLNodePtr newNode = std::make_shared<beachline::BLNode>(
			std::make_pair(this->currentSiteId, this->currentSiteId),
			&this->sweepLinePosition,
			_points);
	return newNode;
}

BLNodePtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::createTree(int index, int index_behind,
																					double *sweepline,
																					const std::vector <Point2D> *points,
																					std::vector <HalfEdgePtr> &edges)
{
	// create nodes corresponding to branching points
	BLNodePtr node1 = std::make_shared<BLNode>(std::make_pair(index_behind, index), sweepline, points);
	BLNodePtr node2 = std::make_shared<BLNode>(std::make_pair(index, index_behind), sweepline, points);


	// create leaf nodes
	BLNodePtr leaf1 = std::make_shared<BLNode>(std::make_pair(index_behind, index_behind), sweepline, points);
	BLNodePtr leaf2 = std::make_shared<BLNode>(std::make_pair(index, index), sweepline, points);
	BLNodePtr leaf3 = std::make_shared<BLNode>(std::make_pair(index_behind, index_behind), sweepline, points);

	// adjust tree connections
	node1->right = node2;
	node2->parent = node1;

	node1->left = leaf1;
	leaf1->parent = node1;

	node2->left = leaf2;
	leaf2->parent = node2;

	node2->right = leaf3;
	leaf3->parent = node2;

	// add halfedges
	std::pair<HalfEdgePtr, HalfEdgePtr> twin_edges = make_twins(index_behind, index);
	node1->edge = twin_edges.first;//second;//first;
	node2->edge = twin_edges.second;//first;//second;

	edges.push_back(twin_edges.first);
	edges.push_back(twin_edges.second);

	// connect leaf nodes
	connect(leaf1, leaf2);
	connect(leaf2, leaf3);

	// reset height of a node
	update_height(node2);
	update_height(node1);

	this->inquire(node1)->printNodeInfo("node1");
	this->inquire(node2)->printNodeInfo("node2");
	this->inquire(leaf1)->printNodeInfo("leaf1");
	this->inquire(leaf2)->printNodeInfo("leaf2");
	this->inquire(leaf3)->printNodeInfo("leaf3");

	// return the result
	return node1;
}

BLNodePtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::createSimpleTree(int index, int index_behind, double *sweepline,
						   const std::vector<Point2D> *points,
						   std::vector<HalfEdgePtr> &edges)
{
	BLNodePtr node, leaf_l, leaf_r;

	std::pair<HalfEdgePtr, HalfEdgePtr> twin_edges = make_twins(index_behind, index);

	edges.push_back(twin_edges.first);
	edges.push_back(twin_edges.second);

	if ((*points)[index].x < (*points)[index_behind].x) {
		// Depends on the point order
		node = std::make_shared<BLNode>(std::make_pair(index, index_behind), sweepline, points);
		leaf_l = std::make_shared<BLNode>(std::make_pair(index, index), sweepline, points);
		leaf_r = std::make_shared<BLNode>(std::make_pair(index_behind, index_behind), sweepline, points);
		node->edge = twin_edges.second;//twin_edges.first;
	} else {
		node = std::make_shared<BLNode>(std::make_pair(index_behind, index), sweepline, points);
		leaf_l = std::make_shared<BLNode>(std::make_pair(index_behind, index_behind), sweepline, points);
		leaf_r = std::make_shared<BLNode>(std::make_pair(index, index), sweepline, points);
		node->edge = twin_edges.first;//twin_edges.second;
	}

	node->left = leaf_l;
	node->right = leaf_r;

	leaf_l->parent = node;
	leaf_r->parent = node;

	connect(leaf_l, leaf_r);
	update_height(node);

	return node;
}



NodeInquirer* openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::inquire(
		beachline::BLNodePtr node)
{
	this->nodeInquirer->setSubject(node);
	return this->nodeInquirer;
}

bool openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::isCurrentNodeRoot()
{
	return (this->currentNode->get_id()==this->root->get_id());
}