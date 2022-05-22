//
// Created by olivier on 11/05/2022.
//

#include "BeachLine.h"

#include <cstring>
#include <memory>
#include <utility>
#include <stack>

#include "Node.hpp"

using namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm;
namespace dataType = openLife::procedure::diagram::voronoi::fortuneAlgorithm::dataType;

/**
 * @note// create a beachline tree
 */
openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::BeachLine(NodeInquirer* nodeInquirer)
{
	this->firstArc = nullptr;
	this->halfEdges = new std::vector<beachline::HalfEdgePtr>();
	this->newEvent = new std::queue<EventPtr>();
	this->nodeInquirer = nodeInquirer;
	this->sweepLinePosition = 0L;
	this->vertex = new std::queue<beachline::VertexPtr>();
}

openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::~BeachLine() {}

/**
 *
 * @param idSite
 * @param newPoint
 */
void openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::addArc(int idSite, Point2D newPoint)
{
	//!
	if(this->firstArc)
	{
		//!
		this->sweepLinePosition = newPoint.y;

		beachline::BLNodePtr arc = this->find(this->firstArc, idSite, newPoint);//!getClosestNode ?

		// check number of intersection points
		int isp_num = intersectionPointsNum(this->sitePoint->at(arc->get_id()), newPoint, newPoint.y);

		// different subtrees depending on the number of intersection points
		beachline::BLNodePtr subtree, left_leaf, right_leaf;
		if (isp_num == 1)
		{
			subtree = this->createSimpleTree(idSite, arc->get_id(), &(newPoint.y), this->sitePoint, *(this->halfEdges));
			left_leaf = subtree->left;
			right_leaf = subtree->right;
		}
		else if (isp_num == 2)
		{
			subtree = this->createTree(idSite, arc->get_id(), &(newPoint.y), this->sitePoint, *(this->halfEdges));
			left_leaf = subtree->left;
			right_leaf = subtree->right->right;
		}
		else return;

		if (arc->prev != nullptr) beachline::connect(arc->prev, left_leaf);
		if (arc->next != nullptr) beachline::connect(right_leaf, arc->next);

		this->firstArc = this->replace(arc, subtree);// Replace old leaf with a subtree and rebalance it


		//!create detect CircleEvent
		// Check circle events //TODO: createCircleEvent cheeck in arc builder
		if (arc->circle_event != nullptr)//TODO: bind circleEvent to node <=> arc->circle_event.get() != nullptr
		{
			printf("\n\tbind circle event to arcNode id:%i",arc->get_id());
			EventPtr circle_e = arc->circle_event;
			circle_e->type = Event::SKIP; // ignore corresponding event
		}

		EventPtr circle_event = checkCircleEvent(left_leaf->prev, left_leaf, left_leaf->next, *this->sitePoint, newPoint.y);
		if (circle_event != nullptr)
		{
			this->newEvent->push(circle_event);
		}
		circle_event = checkCircleEvent(right_leaf->prev, right_leaf, right_leaf->next, *this->sitePoint, newPoint.y);
		if (circle_event != nullptr)
		{
			this->newEvent->push(circle_event);
		}
	}
	else
	{
		//!init empty beach line tree
		this->firstArc = this->createNode(
				-1,
				idSite,
				newPoint,
				idSite,
				idSite);
	}
}

void openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::moveToEdgeEndPoint(Point2D newPoint, beachline::BLNodePtr arc, Point2D center)
{
	this->sweepLinePosition = newPoint.y;

	//!get breakpoint nodes
	std::pair<beachline::BLNodePtr, beachline::BLNodePtr> breakpoints = this->getBreakpoints(arc);

	//if(!this->isValidBreakPoints(breakpoints))return;

	this->breakpoints = breakpoints;

	return;
	/******************************************************************************************************************


	//this->vertex->clear();
	//!create vertex
	// create a new vertex and insert into doubly-connected edge list
	beachline::VertexPtr vertex = std::make_shared<beachline::Vertex>(center);//center from function parameters
	printf("\n\tcreate vertex {id:? coord(%f, %f)}",vertex->point.x, vertex->point.y);

	// store vertex of Voronoi diagram
	this->vertex->push(vertex);
	/******************************************************************************************************************

	// remove circle event corresponding to next leaf
	if (arc->prev != nullptr && arc->prev->circle_event != nullptr)
	{
		EventPtr circle_e = arc->prev->circle_event;
		circle_e->type = Event::SKIP; // ignore corresponding event
	}

	// remove circle event corresponding to prev leaf
	if (arc->next != nullptr && arc->next->circle_event != nullptr)
	{
		EventPtr circle_e = arc->next->circle_event;
		circle_e->type = Event::SKIP; // ignore corresponding event
	}

	// store pointers to the next and previous leaves
	beachline::BLNodePtr prev_leaf, next_leaf;
	prev_leaf = arc->prev;
	next_leaf = arc->next;

	// They should not be null
	assert(prev_leaf != nullptr);
	assert(next_leaf != nullptr);

	// get node associated with a new edge
	beachline::BLNodePtr new_edge_node;
	if (arc->parent == breakpoints.first)
		new_edge_node = breakpoints.second;
	else
		new_edge_node = breakpoints.first;

	// remove arc from the beachline
	this->firstArc = beachline::remove(arc);

	// make a new pair of halfedges
	std::pair<beachline::HalfEdgePtr, beachline::HalfEdgePtr> twin_nodes = beachline::make_twins(prev_leaf->get_id(), next_leaf->get_id());
	new_edge_node->edge = twin_nodes.first;
	//1/ new_edge_node->edge = twin_nodes.first;
	/******************************************************************************************************************

	//!Create halfEdges
	//this->halfEdges->clear();
	beachline::HalfEdgePtr h_first = breakpoints.first->edge;
	beachline::HalfEdgePtr h_second = breakpoints.second->edge;

	// connect halfedges
	beachline::connect_halfedges(h_second, h_first->twin);
	beachline::connect_halfedges(h_first, twin_nodes.first);
	beachline::connect_halfedges(twin_nodes.second, h_second->twin);

	// halfedges are pointing into a vertex  -----> O <-----
	// not like this <---- O ----->
	// counterclockwise
	h_first->vertex = vertex;
	h_second->vertex = vertex;
	twin_nodes.second->vertex = vertex;
	vertex->edge = h_second;

	this->halfEdges->push_back(twin_nodes.first);
	this->halfEdges->push_back(twin_nodes.second);
	/******************************************************************************************************************

	//!generate new circleEvent id any
	printf("\n\teventStackSize: %i", this->newEvent->size());
	// check new circle events
	if (prev_leaf != nullptr && next_leaf != nullptr)
	{
		EventPtr circle_event = checkCircleEvent(prev_leaf->prev, prev_leaf, next_leaf, *this->sitePoint, this->sweepLinePosition);
		if (circle_event != nullptr)
		{
			this->newEvent->push(circle_event);
		}
		circle_event = checkCircleEvent(prev_leaf, next_leaf, next_leaf->next, *this->sitePoint, this->sweepLinePosition);
		if (circle_event != nullptr)
		{
			this->newEvent->push(circle_event);
		}
	}
	/******************************************************************************************************************/
	printf(" => %i", this->newEvent->size());
}

/**
 *
 * @note Returns breakpoints for a given arc
 */
std::pair<BLNodePtr, BLNodePtr> openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::getBreakpoints(BLNodePtr leaf)
{
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

beachline::VertexPtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::getEdge() {}

BLNodePtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::createNode(
		int type,
		int index,
		Point2D point,
		int ind1,
		int ind2)
{
	BLNodePtr newNode = std::make_shared<BLNode>(std::make_pair(ind1, ind2), index, point);
	newNode->focal.index = index;
	newNode->focal.point = point;

	return newNode;
}

void openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::setPoints(std::vector <Point2D> *points)
{
	this->sitePoint = points;
}

void openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::setHalfEdges(
		std::vector <beachline::HalfEdgePtr> *halfEdges)
{
	this->halfEdges = halfEdges;
}

void openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::setSitePointLimitValues(
		double xMin,
		double xMax,
		double yMin,
		double yMax)
{
	this->sitePointLimit.x.min = xMin;
	this->sitePointLimit.x.max = xMax;
	this->sitePointLimit.y.min = yMin;
	this->sitePointLimit.y.max = yMax;
}

BLNodePtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::createTree(int index, int index_behind,
																					double *sweepline,
																					const std::vector <Point2D> *points,
																					std::vector <HalfEdgePtr> &edges)
{
	printf("\n\t=====>createTree");
	// create nodes corresponding to branching points
	BLNodePtr node1 = this->createNode(
			-1,
			-1,
			{0, 0},
			index_behind,
			index);

	BLNodePtr node2 = this->createNode(
			-1,
			-1,
			{0, 0},
			index,
			index_behind);


	// create leaf nodes
	BLNodePtr leaf1 = this->createNode(
			-1,
			-1,
			{0, 0},
			index_behind,
			index_behind);

	BLNodePtr leaf2 = this->createNode(
			-1,
			-1,
			{0, 0},
			index,
			index);

	BLNodePtr leaf3 = this->createNode(
			-1,
			-1,
			{0, 0},
			index_behind,
			index_behind);

	// adjust tree connections
	node1->right = node2;
	node1->left = leaf1;

	node2->parent = node1;
	node2->left = leaf2;
	node2->right = leaf3;

	leaf1->parent = node1;
	leaf2->parent = node2;
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

	// return the result
	return node1;
}

BLNodePtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::createSimpleTree(int index, int index_behind, double *sweepline,
						   const std::vector<Point2D> *points,
						   std::vector<HalfEdgePtr> &edges)
{
	printf("\n\t=====>createSimpleTree");
	BLNodePtr node, leaf_l, leaf_r;

	std::pair<HalfEdgePtr, HalfEdgePtr> twin_edges = make_twins(index_behind, index);

	edges.push_back(twin_edges.first);
	edges.push_back(twin_edges.second);

	if ((*points)[index].x < (*points)[index_behind].x)
	{
		// Depends on the point order
		node = this->createNode(
				-1,
				-1,
				{0, 0},
				index,
				index_behind);

		leaf_l = this->createNode(
				-1,
				-1,
				{0, 0},
				index,
				index);

		leaf_r = this->createNode(
				-1,
				-1,
				{0, 0},
				index_behind,
				index_behind);

		node->edge = twin_edges.second;//twin_edges.first;
	}
	else
	{
		node = this->createNode(
				-1,
				-1,
				{0, 0},
				index_behind,
				index);

		leaf_l = this->createNode(
				-1,
				-1,
				{0, 0},
				index_behind,
				index_behind);

		leaf_r = this->createNode(
				-1,
				-1,
				{0, 0},
				index,
				index);

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

/**
 *
 *
 * @note Find a leaf in a tree such that x is under the parabolic arc, which corresponds to this leaf.
 */
BLNodePtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::find(BLNodePtr root, int idSite, Point2D point)
{
	if (root == nullptr)
	{
		return nullptr;
	}

	double x = point.x;
	printf("\n\tsearch for x=%.1f", x);
	BLNodePtr node = root;
	this->inquire(node)->printNodeInfo("\n\t\t=>search in");
	while (!node->is_leaf())
	{
		int nodeValue = this->getSweepLineEquidistantPointFromFoci(node);
		if (nodeValue < x)
		{
			printf(" test node [%i]{}: [%i].x=%.1f < [middle].x=%i < [%i].x=%.1f  ===> right",
		  			node->get_id(),
				   	node->focal.index,
		  			node->focal.point.x,
		  			nodeValue,
		  			idSite,
		  			x);
			node = node->right;
		}
		else
		{
			printf(" test: [%i].x=%.1f <= %i=nodeValue[%i] ===> left", idSite, x,  nodeValue, node->get_id());
			node = node->left;
		}
		this->inquire(node)->printNodeInfo("\n\t\t=>search in");
	}

	printf("\n\tdump: ");
	for(BLNodePtr node=this->firstArc;!node->is_leaf();node=node->right)
	{
		this->inquire(node)->printNodeInfo("\n\t\t=>");
		this->inquire(node->left)->printNodeInfo("\n\t\t\t=>");
	}

	return node;
}


/**
 *
 * @param node
 * @return
 * @note // Return x-coordinate of:
			//  - in case of leaf node - corresponding focus of parabola;
			//  - in case of internal node - breakpoint;
 */
double openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::getSweepLineEquidistantPointFromFoci(BLNodePtr node)
{
	if (this->sitePoint == nullptr)// || !this->sitePoint.size()
	{
		return std::numeric_limits<double>::infinity();
	}
	if (node->is_leaf())
	{
		//return (*points)[indices.first].x;
		return this->sitePoint->at(node->indices.first).x;
	}
	else
	{
		Point2D p1 = this->sitePoint->at(node->indices.first);//!self idx
		Point2D p2 = this->sitePoint->at(node->indices.second);//!neighbor idx

		//!get points on sweepLine at the same distance from p1 and p2 order by x coord
		std::vector<Point2D> ips = this->getParabolasIntersections(p1, p2);
		if (ips.size() == 2)
		{
			if (p1.y < p2.y)//!the smaller p.y is the earlier focal point hab been sweep basically self is older so we return equidistant point on sweepLine inbetween f1 f2
			{
				return ips[0].x;
			}
			else
			{
				return ips[1].x;
			}
		}
		else
		{
			return ips[0].x;
		}
	}
}

/**
 *
 * @return
 */
EventPtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::getNewEvent()
{
	EventPtr newEvent;
	if(!this->newEvent->empty())
	{
		newEvent = this->newEvent->front();
		this->newEvent->pop();
	}
	return newEvent;
}

/**
 *
 * @param focal1
 * @param focal2
 * @return
 * @note Find intersection points of two parabolas with foci `f1` and `f2` and with given by sweepLinePosition
 * Returns  intersection points ordered by x-coordinate
 * basically give the points on the sweepLine that are a the same distance of f1 and f2
 */
std::vector<Point2D> openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::getParabolasIntersections(
		const Point2D &f1,
		const Point2D &f2)
{
	std::vector<Point2D> result;
	double d = this->sweepLinePosition;

	if (fabs(f1.x - f2.x) < POINT_EPSILON)
	{
		double y = 0.5 * (f1.y + f2.y), D = sqrt(d * d - d * (f1.y + f2.y) + f1.y * f2.y);
		result.push_back(Point2D(f1.x - D, y));
		result.push_back(Point2D(f1.x + D, y));
	}
	else if (fabs(f1.y - f2.y) < POINT_EPSILON)
	{
		double x = 0.5 * (f1.x + f2.x);
		result.push_back(Point2D(x, 0.5 * ((x - f1.x) * (x - f1.x) + f1.y * f1.y  - d * d) / (f1.y - d)));
	}
	else
	{

		double D = 2. * sqrt(pow(f1.x - f2.x, 2) * (d - f1.y) * (d - f2.y) * (pow(f1.x - f2.x, 2) + pow(f1.y - f2.y, 2)));
		double T = -2. * d * pow(f1.x - f2.x, 2) + (f1.y + f2.y) * (pow(f2.x - f1.x, 2) + pow(f2.y - f1.y, 2));
		double Q = 2. * pow(f1.y - f2.y, 2);

		double y1 = (T - D) / Q, y2 = (T + D) / Q;
		double x1 = 0.5 * (f1.x * f1.x - f2.x * f2.x + (2 * y1 - f2.y - f1.y) * (f2.y - f1.y)) / (f1.x - f2.x);
		double x2 = 0.5 * (f1.x * f1.x - f2.x * f2.x + (2 * y2 - f2.y - f1.y) * (f2.y - f1.y)) / (f1.x - f2.x);

		if (x1 > x2)
		{
			std::swap(x1, x2);
			std::swap(y1, y2);
		}
		result.push_back(Point2D(x1, y1));
		result.push_back(Point2D(x2, y2));
	}

	return result;
}

/**
 *
 * @return
 */
std::queue<beachline::VertexPtr>* openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::getVertices()
{
	return this->vertex;
}

std::vector<beachline::HalfEdgePtr> * openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::getHalfEdges()
{
	return this->halfEdges;
}

NodeInquirer* openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::inquire(
		beachline::BLNodePtr node)
{
	this->nodeInquirer->setSubject(node);
	return this->nodeInquirer;
}

bool openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::isEmpty()
{
	return (this->firstArc == nullptr);
}

bool openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::isValidBreakPoints(
		std::pair<beachline::BLNodePtr, beachline::BLNodePtr> breakpoints)
{
	// recheck if it's a false alarm 1
	if (breakpoints.first == nullptr || breakpoints.second == nullptr) return false;

	// recheck if it's a false alarm 2
	double v1 = this->getSweepLineEquidistantPointFromFoci(breakpoints.first);
	double v2 = this->getSweepLineEquidistantPointFromFoci(breakpoints.second);

	if (fabs(v1 - v2) > BREAKPOINTS_EPSILON) return false;

	return true;
}

/**
 *
 * @param node
 * @param new_node
 * @return
 * @note Replace a leaf `node` with a new subtree, which has root `new_node`.
 * The function rebalances the tree and returns the pointer to a new root node.
 */
BLNodePtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::replace(BLNodePtr node, BLNodePtr new_node)
{
	if (node == nullptr) {
		return new_node;
	}

	// Find x-coordinate
	double x = this->getSweepLineEquidistantPointFromFoci(new_node);

	// Get a parent node
	BLNodePtr parent_node = node->parent;

	// Remove leaf, because it's replaced by a new subtree
	//        delete node;

	// Insert the node
	new_node->parent = parent_node;
	if (parent_node != nullptr) {
		if (this->getSweepLineEquidistantPointFromFoci(parent_node) < x)
		{
			parent_node->right = new_node;
		}
		else
		{
			parent_node->left = new_node;
		}
	}

	// Rebalance the tree
	node = new_node;
	while (parent_node != nullptr)
	{
		update_height(parent_node);
		int balance = get_balance(parent_node);
		if (balance > 1) { // left subtree is higher than right subtree by more than 1
			if (parent_node->left != nullptr && !parent_node->left->is_leaf() && get_balance(parent_node->left) < 0) { // @TODO ensure that
				parent_node->left = rotate_left(parent_node->left);
			}
			parent_node = rotate_right(parent_node);
		} else if (balance < -1) { // right subtree is lower than left subtree by more than 1
			if (parent_node->right != nullptr && !parent_node->right->is_leaf() && get_balance(parent_node->right) > 0) {
				parent_node->right = rotate_right(parent_node->right);
			}
			parent_node = rotate_left(parent_node);
		}

		//_validate(parent_node);

		node = parent_node;
		parent_node = parent_node->parent;
	}

	//_check_balance(node);

	return node;
}

void openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::setQueueEvent(
		openLife::procedure::diagram::voronoi::fortuneAlgorithm::EventQueue *eventQueue)
{
	this->eventQueue = eventQueue;
}
