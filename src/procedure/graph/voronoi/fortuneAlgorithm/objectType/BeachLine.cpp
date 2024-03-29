//
// Created by olivier on 11/05/2022.
//

#include "BeachLine.h"

#include <cstring>
#include <memory>
#include <utility>
#include <stack>

#include "../Debug.h"
#include "Node.hpp"

using namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm;
namespace NS = openLife::procedure::diagram::voronoi;
namespace FA = openLife::procedure::diagram::voronoi::fortuneAlgorithm;

/**
 * @note// create a beachline tree
 */
openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::BeachLine(NodeInquirer* nodeInquirer)
{
	this->firstArc = nullptr;
	this->cellEdge = new std::vector<FA::dataType::EdgePtr>();
	this->cellEdgeEnd = new std::vector<FA::dataType::EdgeEndPtr>();
	this->cellSite = new std::vector<FA::dataType::SitePtr>();
	this->halfEdges = new std::vector<beachline::HalfEdgePtr>();
	this->newEvent = new std::queue<EventPtr>();
	this->nodeInquirer = nodeInquirer;
	this->sweepLinePosition = 0L;
	//this->vertex = new std::queue<beachline::VertexPtr>();
}

openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::~BeachLine() {}

/**********************************************************************************************************************/
//!PUBLIC

std::vector<FA::dataType::EdgePtr>* openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::getEdges()
{
	return this->cellEdge;
}

/**
 *
 * @param newPoint
 * @param arc
 * @param center
 */
void openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::moveToEdgeEndPoint(Point2D newPoint, beachline::BLNodePtr arc, Point2D center)
{
	this->sweepLinePosition = newPoint.y;

	//!get breakpoint nodes
	std::pair<beachline::BLNodePtr, beachline::BLNodePtr> breakpoints = this->getBreakpoints(arc);
	//NS::Debug::inquire(arc)->printNodeInfo("\n\t\tleaf 1:");
	//NS::Debug::inquire(arc->next)->printNodeInfo("\n\t\tleaf 2:");
	//NS::Debug::inquire(arc->prev)->printNodeInfo("\n\t\tleaf 3:");


	if(!this->isValidBreakPoints(breakpoints))return;

	//!create a new vertex and insert into doubly-connected edge list
	/*
	beachline::VertexPtr vertex = std::make_shared<beachline::Vertex>(center);//center from function parameters
	printf("\n\tcreate vertex [-](%.1f, %.1f)}",vertex->point.x, vertex->point.y);
*/

	//!remove circle event corresponding to next & prev leaf
	if (arc->prev != nullptr && arc->prev->circle_event != nullptr) {
		EventPtr circle_e = arc->prev->circle_event;
		circle_e->type = Event::SKIP; // ignore corresponding event
	}
	if (arc->next != nullptr && arc->next->circle_event != nullptr) {
		EventPtr circle_e = arc->next->circle_event;
		circle_e->type = Event::SKIP; // ignore corresponding event
	}

	//this->vertex = vertex;

	//!store pointers to the next and previous leaves
	beachline::BLNodePtr prev_leaf, next_leaf;
	prev_leaf = arc->prev;
	next_leaf = arc->next;

	// They should not be null
	assert(prev_leaf != nullptr);
	assert(next_leaf != nullptr);

	//!
	FA::dataType::Edge* edge;
	edge = this->getOrCreateEdge(this->inquire(prev_leaf)->getSite(), this->inquire(arc)->getSite());
	edge->setEdgeEnd(this->getOrCreateEdgeEnd(center).get());

	edge = this->getOrCreateEdge(this->inquire(arc)->getSite(), this->inquire(next_leaf)->getSite());
	edge->setEdgeEnd(this->getOrCreateEdgeEnd(center).get());

	edge = this->getOrCreateEdge(this->inquire(next_leaf)->getSite(), this->inquire(prev_leaf)->getSite());
	edge->setEdgeEnd(this->getOrCreateEdgeEnd(center).get());


	beachline::HalfEdgePtr h_first = breakpoints.first->edge;
	beachline::HalfEdgePtr h_second = breakpoints.second->edge;

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

	// connect halfedges
	beachline::connect_halfedges(h_second, h_first->twin);
	beachline::connect_halfedges(h_first, twin_nodes.first);
	beachline::connect_halfedges(twin_nodes.second, h_second->twin);

	// halfedges are pointing into a vertex  -----> O <-----
	// not like this <---- O ----->
	// counterclockwise
	/*
	h_first->vertex = vertex;
	h_second->vertex = vertex;
	twin_nodes.second->vertex = vertex;
	vertex->edge = h_second;
	*/

	this->halfEdges->push_back(twin_nodes.first);
	this->halfEdges->push_back(twin_nodes.second);

	// check new circle events
	if (prev_leaf != nullptr && next_leaf != nullptr) {
		EventPtr circle_event = this->checkCircleEvent(prev_leaf->prev, prev_leaf, next_leaf);
		if (circle_event != nullptr) {
			this->newEvent->push(circle_event);
		}
		circle_event = this->checkCircleEvent(prev_leaf, next_leaf, next_leaf->next);
		if (circle_event != nullptr) {
			this->newEvent->push(circle_event);
		}
	}
}

/**
 *
 * @param newPoint
 */
void openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::moveToSitePoint(Point2D newPoint)
{
	//!
	FA::dataType::SitePtr currentSite = this->createSitePoint(newPoint);

	//!
	if(this->firstArc)
	{
		//!
		this->sweepLinePosition = currentSite->point.y;
		beachline::BLNodePtr neighborSiteNode = this->getFirstNeighborSiteNode(currentSite);
		//NS::Debug::inquire(neighborSiteNode)->printNodeInfo("\n\tfound neighbor:");//!debug
		this->createEdge(currentSite.get(), this->inquire(neighborSiteNode)->getSite());

		//!create edge node structure
		//check number of intersection points
		int isp_num = this->getParabolasIntersectionNumber(
				this->inquire(neighborSiteNode)->getSite()->point,
				currentSite->point);

		// different subtrees depending on the number of intersection points
		beachline::BLNodePtr subtree, left_leaf, right_leaf;
		if (isp_num == 1)
		{
			subtree = this->createSimpleTree(currentSite->id, neighborSiteNode->get_id(), *(this->halfEdges));
			left_leaf = subtree->left;
			right_leaf = subtree->right;
		}
		else if (isp_num == 2)
		{
			subtree = this->createTree(currentSite->id, neighborSiteNode->get_id(), *(this->halfEdges));
			left_leaf = subtree->left;
			right_leaf = subtree->right->right;
		}
		else return;

		if (neighborSiteNode->prev != nullptr) beachline::connect(neighborSiteNode->prev, left_leaf);
		if (neighborSiteNode->next != nullptr) beachline::connect(right_leaf, neighborSiteNode->next);
		this->replace(neighborSiteNode, subtree);// Replace old leaf with a subtree and rebalance it
		/**************************************************************************************************************/


		//!create detect CircleEvent
		// Check circle events //TODO: createCircleEvent check in arc builder
		if (neighborSiteNode->circle_event != nullptr)
		{
			EventPtr circle_e = neighborSiteNode->circle_event;
			circle_e->type = Event::SKIP; // ignore corresponding event
		}

		EventPtr circle_event = this->checkCircleEvent(left_leaf->prev, left_leaf, left_leaf->next);//check for all node around the center
		if (circle_event != nullptr)
		{
			this->newEvent->push(circle_event);
		}
		circle_event = this->checkCircleEvent(right_leaf->prev, right_leaf, right_leaf->next);
		if (circle_event != nullptr)
		{
			this->newEvent->push(circle_event);
		}
	}
	else
	{
		//!init empty beach line tree
		this->firstArc = this->createNode(
				FA::dataType::beachLine::SITE,
				currentSite->id,//currentSite.id
				currentSite->id);
	}
}

/**********************************************************************************************************************/
//!PRIVATE

EventPtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::checkCircleEvent(
		beachline::BLNodePtr n1,
		beachline::BLNodePtr n2,
		beachline::BLNodePtr n3)
{
	if (n1 == nullptr || n2 == nullptr || n3 == nullptr)
		return nullptr;

	Point2D p1 = this->cellSite->at(n1->get_id())->point;//points[n1->get_id()];
	Point2D p2 = this->cellSite->at(n2->get_id())->point;//points[n2->get_id()];
	Point2D p3 = this->cellSite->at(n3->get_id())->point;//points[n3->get_id()];
	Point2D center, bottom;

	if (p2.y > p1.y && p2.y > p3.y)
		return nullptr;

	if (!this->findCircleCenter(p1, p2, p3, center))
		return nullptr;


	bottom = center;
	bottom.y += (center - p2).norm();
	// check circle event
	if (fabs(bottom.y - this->sweepLinePosition) < POINT_EPSILON || this->sweepLinePosition < bottom.y)
	{
		//!
		// create a circle event structure
		EventPtr e = std::make_shared<Event>(-1, Event::CIRCLE, bottom);
		// initialize attributes
		e->center = center;
		e->arc = n2;
		// add reference in the corresponding node
		n2->circle_event = e;
		return e;
	}

	return nullptr;
}

/**
 *
 * @param site1
 * @param site2
 * @return
 */
FA::dataType::EdgePtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::createEdge(
		FA::dataType::Site* site1,
		FA::dataType::Site* site2)
{
	FA::dataType::EdgePtr edge = std::make_shared<FA::dataType::Edge>();
	edge->id = this->cellEdge->size();
	edge->site[0] = site1;
	edge->site[1] = site2;
	edge->end[0] = nullptr;
	edge->end[1] = nullptr;
	this->cellEdge->push_back(edge);
	//NS::Debug::inquire(edge.get())->printInfo("\n\tcreateEdge:");
	return edge;
}

/**
 *
 * @param point
 * @return
 */
FA::dataType::EdgeEndPtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::createEdgeEnd(
		Point2D point)
{
	FA::dataType::EdgeEndPtr edgeEnd = std::make_shared<FA::dataType::EdgeEnd>();
	edgeEnd->id = this->cellEdgeEnd->size();
	edgeEnd->point.x = point.x;
	edgeEnd->point.y = point.y;
	this->cellEdgeEnd->push_back(edgeEnd);
	//printf("\n\tcreateEdgeEnd: [%i](%.1f, %.1f)", edgeEnd->id, edgeEnd->point.x, edgeEnd->point.y);
	return edgeEnd;
}

/**
 *
 * @param nodeType
 * @param currentSite
 * @param ind1
 * @param ind2
 * @return
 */
BLNodePtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::createNode(
		FA::dataType::beachLine::NodeType nodeType,
		int ind1,
		int ind2)
{
	BLNodePtr newNode = std::make_shared<BLNode>(std::make_pair(ind1, ind2));
	switch(nodeType)
	{
		case FA::dataType::beachLine::NodeType::SITE:
			newNode->type = nodeType;
			newNode->reference = (void*)this->cellSite->at(ind1).get();
			break;
		default:
			newNode->type = nodeType;
			newNode->reference = (void*)this->cellSite->at(ind1).get();
			break;
	}
	//NS::Debug::inquire(newNode)->printNodeInfo("\n\tcreate node:");
	return newNode;
}

BLNodePtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::createSimpleTree(
		int index,
		int index_behind,
		std::vector<HalfEdgePtr> &edges)
{
	//printf("\n\t=====>createSimpleTree");
	BLNodePtr node, leaf_l, leaf_r;

	std::pair<HalfEdgePtr, HalfEdgePtr> twin_edges = make_twins(index_behind, index);

	edges.push_back(twin_edges.first);
	edges.push_back(twin_edges.second);

	if (this->cellSite->at(index)->point.x < this->cellSite->at(index_behind)->point.x)
	{
		// Depends on the point order
		node = this->createNode(
				FA::dataType::beachLine::NodeType::UNDEFINED,
				index,
				index_behind);

		leaf_l = this->createNode(
				FA::dataType::beachLine::NodeType::SITE,
				index,
				index);

		leaf_r = this->createNode(
				FA::dataType::beachLine::NodeType::SITE,
				index_behind,
				index_behind);

		node->edge = twin_edges.second;//twin_edges.first;
	}
	else
	{
		node = this->createNode(
				FA::dataType::beachLine::NodeType::UNDEFINED,
				index_behind,
				index);

		leaf_l = this->createNode(
				FA::dataType::beachLine::NodeType::SITE,
				index_behind,
				index_behind);

		leaf_r = this->createNode(
				FA::dataType::beachLine::NodeType::SITE,
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

FA::dataType::SitePtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::createSitePoint(
		Point2D point)
{
	FA::dataType::SitePtr site = std::make_shared<FA::dataType::Site>();
	site->id = this->cellSite->size();
	site->point = point;
	this->cellSite->push_back(site);
	return site;
}

BLNodePtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::createTree(
		int index,
		int index_behind,
		std::vector <HalfEdgePtr> &edges)
{
	// create nodes corresponding to branching points
	BLNodePtr node1 = this->createNode(
			FA::dataType::beachLine::NodeType::UNDEFINED,
			index_behind,
			index);

	BLNodePtr node2 = this->createNode(
			FA::dataType::beachLine::NodeType::UNDEFINED,
			index,
			index_behind);


	// create leaf nodes
	BLNodePtr leaf1 = this->createNode(
			FA::dataType::beachLine::NodeType::SITE,
			index_behind,
			index_behind);

	BLNodePtr leaf2 = this->createNode(
			FA::dataType::beachLine::NodeType::SITE,
			index,
			index);

	BLNodePtr leaf3 = this->createNode(
			FA::dataType::beachLine::NodeType::SITE,
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

/**
 *
 * @param p1
 * @param p2
 * @param p3
 * @param center
 * @return
 * @note Find a center of a circle with given three points.
 Returns false if points are collinear.
 Otherwise returns true and updates x- and y-coordinates of the `center` of circle.
 */
bool openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::findCircleCenter(
		const Point2D &p1,
		const Point2D &p2,
		const Point2D &p3,
		Point2D &center)
{
	// get normalized vectors
	Point2D u1 = (p1 - p2).normalized(), u2 = (p3 - p2).normalized();

	double cross = crossProduct(u1, u2);

	// check if vectors are collinear
	if (fabs(cross) < CIRCLE_CENTER_EPSILON) {
		return false;
	}

	// get central points
	Point2D pc1 = 0.5 * (p1 + p2), pc2 = 0.5 * (p2 + p3);

	// get free components
	double b1 = dotProduct(u1, pc1), b2 = dotProduct(u2, pc2);

	// calculate the center of a circle
	center.x = (b1 * u2.y - b2 * u1.y) / cross;
	center.y = (u1.x * b2 - u2.x * b1) / cross;
	return true;
}

/**
 *
 * @note Returns breakpoints for a given arc
 */
std::pair<BLNodePtr, BLNodePtr> openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::getBreakpoints(
		BLNodePtr leaf)
{
	if (leaf == nullptr || leaf->next == nullptr || leaf->prev == nullptr)
		return std::make_pair<BLNodePtr>(nullptr, nullptr);

	BLNodePtr parent = leaf->parent, gparent = leaf->parent;
	std::pair<int,int> bp1(leaf->prev->get_id(), leaf->get_id()); // left breakpoint
	std::pair<int,int> bp2(leaf->get_id(), leaf->next->get_id()); // right breakpoint
	std::pair<int,int> other_bp;

	bool left_is_missing = true;

	if (parent->has_indices(bp1))
	{
		other_bp = bp2;
		left_is_missing = false;
	}
	else if (parent->has_indices(bp2))
	{
		other_bp = bp1;
		left_is_missing = true;
	}

	// Go up and rebalance the whole tree
	while (gparent != nullptr)
	{
		if (gparent->has_indices(other_bp))
		{
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

/**
 *
 * @param site1
 * @param site2
 * @return
 */
FA::dataType::Edge* openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::getEdge(
		FA::dataType::Site site1,
		FA::dataType::Site site2)
{
	FA::dataType::Edge* edge = nullptr;
	for(unsigned int i=0; i<this->cellEdge->size(); i++)
	{
		if((this->cellEdge->at(i)->site[0]->id==site1.id && this->cellEdge->at(i)->site[1]->id==site2.id)
		|| (this->cellEdge->at(i)->site[0]->id==site2.id && this->cellEdge->at(i)->site[1]->id==site1.id))
		{
			edge = this->cellEdge->at(i).get();
			break;
		}
	}
	return edge;
}

FA::dataType::EdgeEndPtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::getEdgeEnd(
		Point2D point)
{
	for(size_t i=0; i<this->cellEdgeEnd->size(); i++)
	{
		if((fabs(point.x - this->cellEdgeEnd->at(i)->point.x) < POINT_EPSILON)
			&& (fabs(point.y - this->cellEdgeEnd->at(i)->point.y) < POINT_EPSILON))
		{
			return this->cellEdgeEnd->at(i);
		}
	}
	return FA::dataType::EdgeEndPtr(nullptr);
}

/**
 *
 *
 * @note Find a leaf in a tree such that x is under the parabolic arc, which corresponds to this leaf.
 */
BLNodePtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::getFirstNeighborSiteNode(
		FA::dataType::SitePtr localSite)
{
	double x = localSite->point.x;
	BLNodePtr node = this->firstArc;
	while (!node->is_leaf())
	{
		int nodeValue = this->getSweepLineEquidistantPointFromFoci(node);

		if (nodeValue < x)
		{
			node = node->right;
		}
		else
		{
			node = node->left;
		}
	}
	return node;
}

std::vector<beachline::HalfEdgePtr>* openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::getHalfEdges()
{
	return this->halfEdges;
}

FA::dataType::Edge* openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::getOrCreateEdge(
		FA::dataType::Site* site1,
		FA::dataType::Site* site2)
{
	FA::dataType::Edge* edge;
	edge = this->getEdge(*site1, *site2);
	if(edge == nullptr)
	{
		edge = this->createEdge(site1, site2).get();
	}
	return edge;
}

FA::dataType::EdgeEndPtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::getOrCreateEdgeEnd(
		Point2D point)
{
	if(this->getEdgeEnd(point) == nullptr)
	{
		return this->createEdgeEnd(point);
	}
	else return this->getEdgeEnd(point);
}

/**
 *
 * @param focal1
 * @param focal2
 * @return
 * @note Calculate number of intersection points between two parabolas with foci `f1` and `f2` and with given `directrix`
 */
unsigned int openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::getParabolasIntersectionNumber(
		const Point2D &f1,
		const Point2D &f2)
{
	if (fabs(f1.x - f2.x) < POINT_EPSILON && fabs(f1.y - f2.y) < POINT_EPSILON) {
		return 0;
	}
	if (fabs(f1.y - f2.y) < POINT_EPSILON)
		return 1;
	return 2;
}

/**
 *
 * @param node
 * @return
 * @note
 * def : p for general point with coord (p.x,p.y).
 * def : X is index (ie 1, 2, ..., 9, A, ..., Z) so pX for any specific point with coord (pX.x,pX.y)
 * def : f for focal/site point, i for parabola intersection point
 * def : sweepLine.y value is current maximum p.y value
 * def : iX for intersection point. x1 and x2 assert (x1.x<x2.x)
 * => if (f1.y!=f2.y) then 2 points p can be found on sweepLine where d(f1,p)==d(f2,p)
 * def : focal "fX" => "fX=(f1 or f2)" and "fX.y=max(f1.y,f2.y)"
 * => bigger "p.y" value mean finer parabola so i1.x<fX.x<i2.x
 * => if (p1.y<p2.y) then i1.x<p2.x<i2.x then if (p1.x<p2.x) then d(p1.x,i1.x)<d(p1.x,i2.x) because d(p1.x,p2x)+(p2.x,i2.x)=d(p1.x,i2.x)
 *
 * // Return x-coordinate of:
			//  - in case of leaf node - corresponding focus of parabola;
			//  - in case of internal node - breakpoint;
 */
double openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::getSweepLineEquidistantPointFromFoci(BLNodePtr node)
{
	if (this->cellSite == nullptr)// || !this->cellSite.size()
	{
		return std::numeric_limits<double>::infinity();
	}
	if (node->is_leaf())
	{
		return this->cellSite->at(node->indices.first)->point.x;
	}
	else
	{
		Point2D p1 = this->cellSite->at(node->indices.first)->point;//!self idx
		Point2D p2 = this->cellSite->at(node->indices.second)->point;//!neighbor idx

		//!get points on sweepLine at the same distance from p1 and p2 order by x coord
		std::vector<Point2D> ips = this->getParabolasIntersections(p1, p2);
		if (ips.size() == 2)
		{
			//!the smaller pX.y is, the earlier the focal pX hab been sweep, the further pX is from sweepLine
			//!the closest pX is from the sweepLine the narrowest is it's parabola
			if (p1.y < p2.y)
			{
				//!i1 and i2 are intersection points so i1.x always smaller than i2.x
				//!If p1.y<p2.y : intersection points (named i1 and i2) should verify p1.y < i1.y i2.y < p2.y
				//!Since p1.y is smaller, p1 has the largest parabola.
				//!Since p2.y is higher, p2 have the finest parabola and if(i1.x<i2.x) i1.x<p2.x<i2.x
				//!Since (p1.y<p2.y) then if(p1.x<p2.x) d(p1.x,i1.x) < d(p1.x,i2.x)
				//!Since (p1.y<p2.y) and d(p2.x,i1.x)==d(p2.x,i2.x) if(p1.x<p2.x) and (i1.x<p2.x<i2.x) then d(p1.x,i1.x)<d(p1.x,i2.x)
				//!basically self is older so we return equidistant point on sweepLine inbetween f1 f2
				//!so p2.x min
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

FA::NodeInquirer* openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::inquire(BLNodePtr node)
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
void openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::replace(
		BLNodePtr node,
		BLNodePtr new_node)
{
	if(node!= nullptr)
	{
		// Find x-coordinate
		double x = this->getSweepLineEquidistantPointFromFoci(new_node);

		// Get a parent node
		BLNodePtr parent_node = node->parent;

		// Remove leaf, because it's replaced by a new subtree
		//        delete node;

		// Insert the node
		new_node->parent = parent_node;
		if (parent_node != nullptr)
		{
			if (this->getSweepLineEquidistantPointFromFoci(parent_node) < x)
			{
				parent_node->right = new_node; //previous
			}
			else
			{
				parent_node->left = new_node; //next
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

		this->firstArc = node;
	}
	else return;
}

void openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine::setHalfEdges(
		std::vector <beachline::HalfEdgePtr> *halfEdges)
{
	this->halfEdges = halfEdges;
}
