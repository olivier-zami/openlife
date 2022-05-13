//
// Created by olivier on 10/05/2022.
//

#include "FortuneAlgorithm.h"

#include <queue>

#include "fortuneAlgorithm/Math/Circle.hpp"
#include "fortuneAlgorithm/objectType/Node.hpp"
#include "fortuneAlgorithm/objectType/BeachLine.h"
#include "fortuneAlgorithm/objectType/EventQueue.h"
#include "fortuneAlgorithm/Voronoi/VoronoiDiagram.hpp"

namespace nsSingleton = openLife::procedure::diagram::voronoi;

openLife::procedure::diagram::voronoi::FortuneAlgorithm* openLife::procedure::diagram::voronoi::FortuneAlgorithm::instance = nullptr;

EventPtr checkCircleEvent(bl::BLNodePtr n1, bl::BLNodePtr n2, bl::BLNodePtr n3,
						  const std::vector<Point2D> &points, double sweepline);


void openLife::procedure::diagram::voronoi::FortuneAlgorithm::generateDiagram(VoronoiDiagram* diagram, const std::vector<Point2D> &points)
{
	if (!nsSingleton::FortuneAlgorithm::instance)
	{
		nsSingleton::FortuneAlgorithm::instance = new openLife::procedure::diagram::voronoi::FortuneAlgorithm();
	}

	nsSingleton::FortuneAlgorithm::instance->addSites(points);
	nsSingleton::FortuneAlgorithm::instance->setOutputDataStruct(diagram);
	nsSingleton::FortuneAlgorithm::instance->sweepAll();
}

openLife::procedure::diagram::voronoi::FortuneAlgorithm::FortuneAlgorithm()
{
	this->beachLine = new openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine();
	this->currentEvent = nullptr;
	this->eventQueue = new openLife::procedure::diagram::voronoi::fortuneAlgorithm::EventQueue();
	this->faces = new std::vector<beachline::HalfEdgePtr>();
	this->halfEdges = new std::vector<beachline::HalfEdgePtr>();
	this->vertices = nullptr;
	this->voronoiSite = new std::vector<Point2D>();
	this->sweepLinePosition = 0L;//!current position of the sweep line
}

openLife::procedure::diagram::voronoi::FortuneAlgorithm::~FortuneAlgorithm() {}

void openLife::procedure::diagram::voronoi::FortuneAlgorithm::addSites(const std::vector<Point2D> &siteCoord)
{
	//!useless for now but might be use
	for(size_t i=0; i<siteCoord.size(); i++)
	{
		Point2D site;
		site.x = siteCoord[i].x;
		site.y = siteCoord[i].y;
		this->voronoiSite->push_back(site);
	}
	this->eventQueue->addSiteEvent(siteCoord);
}

bool openLife::procedure::diagram::voronoi::FortuneAlgorithm::isEveryElementsSweep()
{
	return this->eventQueue->get()->empty();
}

void openLife::procedure::diagram::voronoi::FortuneAlgorithm::setOutputDataStruct(VoronoiDiagram *diagram)
{
	this->vertices = &(diagram->vertices);
}

void openLife::procedure::diagram::voronoi::FortuneAlgorithm::sweepAll()
{
	if(!this->vertices)
	{
		this->vertices = new std::vector<beachline::VertexPtr>();
	}

	// initialize vector of halfedges for faces
	this->faces->resize(this->voronoiSite->size(), nullptr);

	// process events
	while (!this->isEveryElementsSweep())
	{
		printf("\n\n-------------------------------------------------------------------------------------------");
		this->sweepNextElement();

		if (this->currentEvent->type == Event::SITE)// handle site event
		{
			this->beachLine->readSite(this->currentEvent, *(this->voronoiSite));//TODO this->beachLine->readSite(inquire(event)->getSite());

			if(this->beachLine->isCurrentNodeRoot()) continue;

			int point_i = this->currentEvent->index;//TODO event ->getSite().id
			bl::BLNodePtr arc = bl::find(this->beachLine->getRootNode(), this->currentEvent->point.x);//!getClosestNode ?
			bl::BLNodePtr subtree, left_leaf, right_leaf;

			this->beachLine->inquire(arc)->printNodeInfo("getArcNode");

			if (arc->circle_event != nullptr)//TODO: bind circleEvent to node
			{
				printf("\n\tbind circle event to arcNode id:%i",arc->get_id());
				EventPtr circle_e = arc->circle_event;
				circle_e->type = Event::SKIP; // ignore corresponding event
			}

			// check number of intersection points
			int isp_num = intersectionPointsNum(this->voronoiSite->at(arc->get_id()), this->currentEvent->point, this->sweepLinePosition);

			printf("\n\tcheck for interception arc[%i](%f, %f) point(%f, %f) = %i",
				   arc->get_id(),
				   this->voronoiSite->at(arc->get_id()).x,
				   this->voronoiSite->at(arc->get_id()).y,
				   this->currentEvent->point.x,
				   this->currentEvent->point.y,
				   isp_num);

			// different subtrees depending on the number of intersection points
			if (isp_num == 1)
			{
				subtree = this->beachLine->createSimpleTree(point_i, arc->get_id(), &(this->sweepLinePosition), this->voronoiSite, *(this->halfEdges));
				left_leaf = subtree->left;
				right_leaf = subtree->right;
			}
			else if (isp_num == 2)
			{
				subtree = this->beachLine->createTree(point_i, arc->get_id(), &(this->sweepLinePosition), this->voronoiSite, *(this->halfEdges));
				left_leaf = subtree->left;
				right_leaf = subtree->right->right;
			}
			else
			{
				continue;
			}

			if (arc->prev != nullptr)
				bl::connect(arc->prev, left_leaf);

			if (arc->next != nullptr)
				bl::connect(right_leaf, arc->next);

			// Replace old leaf with a subtree and rebalance it
			this->beachLine->root = bl::replace(arc, subtree);

			// Check circle events
			EventPtr circle_event = checkCircleEvent(left_leaf->prev, left_leaf, left_leaf->next, *this->voronoiSite, this->sweepLinePosition);
			if (circle_event != nullptr) {
				this->eventQueue->get()->push(circle_event);
			}
			circle_event = checkCircleEvent(right_leaf->prev, right_leaf, right_leaf->next, *this->voronoiSite, this->sweepLinePosition);
			if (circle_event != nullptr) {
				this->eventQueue->get()->push(circle_event);
			}

		}
		else if (this->currentEvent->type == Event::CIRCLE)
		{ // handle circle event

			bl::BLNodePtr arc = this->currentEvent->arc, prev_leaf, next_leaf;

			// get breakpoint nodes
			std::pair<bl::BLNodePtr, bl::BLNodePtr> breakpoints = this->beachLine->breakpoints(arc);

			// recheck if it's a false alarm 1
			if (breakpoints.first == nullptr || breakpoints.second == nullptr) {
				continue;
			}

			// recheck if it's a false alarm 2
			double v1 = breakpoints.first->value(), v2 = breakpoints.second->value();

			if (fabs(v1 - v2) > BREAKPOINTS_EPSILON) {
				continue;
			}

			// create a new vertex and insert into doubly-connected edge list
			bl::VertexPtr vertex = std::make_shared<bl::Vertex>(this->currentEvent->center);
			printf("\n\tcreate vertext {id:? coord(%f, %f)}",vertex->point.x, vertex->point.y);

			bl::HalfEdgePtr h_first = breakpoints.first->edge;
			bl::HalfEdgePtr h_second = breakpoints.second->edge;

			// store vertex of Voronoi diagram
			this->vertices->push_back(vertex);

			// remove circle event corresponding to next leaf
			if (arc->prev != nullptr && arc->prev->circle_event != nullptr) {
				EventPtr circle_e = arc->prev->circle_event;
				circle_e->type = Event::SKIP; // ignore corresponding event
			}

			// remove circle event corresponding to prev leaf
			if (arc->next != nullptr && arc->next->circle_event != nullptr) {
				EventPtr circle_e = arc->next->circle_event;
				circle_e->type = Event::SKIP; // ignore corresponding event
			}

			// store pointers to the next and previous leaves
			prev_leaf = arc->prev;
			next_leaf = arc->next;

			// They should not be null
			assert(prev_leaf != nullptr);
			assert(next_leaf != nullptr);

			// get node associated with a new edge
			bl::BLNodePtr new_edge_node;
			if (arc->parent == breakpoints.first)
				new_edge_node = breakpoints.second;
			else
				new_edge_node = breakpoints.first;

			// remove arc from the beachline
			this->beachLine->root = bl::remove(arc);

			// make a new pair of halfedges
			std::pair<bl::HalfEdgePtr, bl::HalfEdgePtr> twin_nodes = bl::make_twins(prev_leaf->get_id(), next_leaf->get_id());
			new_edge_node->edge = twin_nodes.first;
			//1/ new_edge_node->edge = twin_nodes.first;

			// connect halfedges
			bl::connect_halfedges(h_second, h_first->twin);
			bl::connect_halfedges(h_first, twin_nodes.first);
			bl::connect_halfedges(twin_nodes.second, h_second->twin);

			// halfedges are pointing into a vertex  -----> O <-----
			// not like this <---- O ----->
			// counterclockwise
			h_first->vertex = vertex;
			h_second->vertex = vertex;
			twin_nodes.second->vertex = vertex;
			vertex->edge = h_second;

			this->halfEdges->push_back(twin_nodes.first);
			this->halfEdges->push_back(twin_nodes.second);

			// check new circle events
			if (prev_leaf != nullptr && next_leaf != nullptr) {
				EventPtr circle_event = checkCircleEvent(prev_leaf->prev, prev_leaf, next_leaf, *this->voronoiSite, this->sweepLinePosition);
				if (circle_event != nullptr) {
					this->eventQueue->get()->push(circle_event);
				}
				circle_event = checkCircleEvent(prev_leaf, next_leaf, next_leaf->next, *this->voronoiSite, this->sweepLinePosition);
				if (circle_event != nullptr) {
					this->eventQueue->get()->push(circle_event);
				}
			}
		}
	}

	// Fill edges corresponding to faces
	for (size_t i = 0; i < this->halfEdges->size(); ++i) {
		bl::HalfEdgePtr he = this->halfEdges->at(i);
		if (he->prev == nullptr || this->faces->at(he->l_index) == nullptr) {
			this->faces->at(he->l_index) = he;
		}
	}
}

void openLife::procedure::diagram::voronoi::FortuneAlgorithm::sweepNextElement()
{
	printf("\n\nmove sweepLine :");
	this->currentEvent = this->eventQueue->getNextEvent();

	// set position of a sweepline
	this->sweepLinePosition = this->currentEvent->point.y;
}
