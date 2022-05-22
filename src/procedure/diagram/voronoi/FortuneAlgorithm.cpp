//
// Created by olivier on 10/05/2022.
//

#include "FortuneAlgorithm.h"

#include <queue>

#include "fortuneAlgorithm/dataType/Event.h"
#include "fortuneAlgorithm/Debug.h"
#include "fortuneAlgorithm/Math/Circle.hpp"
#include "fortuneAlgorithm/objectType/Node.hpp"
#include "fortuneAlgorithm/objectType/BeachLine.h"
#include "fortuneAlgorithm/objectType/EventQueue.h"
#include "fortuneAlgorithm/Voronoi/VoronoiDiagram.hpp"

namespace FA = openLife::procedure::diagram::voronoi::fortuneAlgorithm;
namespace nsSingleton = openLife::procedure::diagram::voronoi;
using namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm;

openLife::procedure::diagram::voronoi::FortuneAlgorithm* openLife::procedure::diagram::voronoi::FortuneAlgorithm::instance = nullptr;

EventPtr checkCircleEvent(beachline::BLNodePtr n1, beachline::BLNodePtr n2, beachline::BLNodePtr n3,
						  const std::vector<Point2D> &points, double sweepline);


/**
 *
 * @param diagram
 * @param points
 */
void openLife::procedure::diagram::voronoi::FortuneAlgorithm::generateDiagram(VoronoiDiagram* diagram, const std::vector<Point2D> &points)
{
	if (!nsSingleton::FortuneAlgorithm::instance)
	{
		nsSingleton::FortuneAlgorithm::instance = new openLife::procedure::diagram::voronoi::FortuneAlgorithm();
	}

	nsSingleton::FortuneAlgorithm::instance->addSites(points);
	nsSingleton::FortuneAlgorithm::instance->setOutputDataStruct(diagram);
	nsSingleton::FortuneAlgorithm::instance->buildDiagram();
}

/**
 *
 */
openLife::procedure::diagram::voronoi::FortuneAlgorithm::FortuneAlgorithm()
{
	//!
	this->dimension = {0L,0L};
	this->eventQueue = new openLife::procedure::diagram::voronoi::fortuneAlgorithm::EventQueue();
	this->faces = new std::vector<beachline::HalfEdgePtr>();
	this->halfEdges = new std::vector<beachline::HalfEdgePtr>();
	this->vertices = nullptr;
	this->sitePoint = new std::vector<Point2D>();
	this->sweepLinePosition = 0L;//!current position of the sweep line

	//!
	this->nodeInquirer = new openLife::procedure::diagram::voronoi::fortuneAlgorithm::NodeInquirer();
	this->nodeInquirer->setSitePointsReference(this->sitePoint);
	this->nodeInquirer->setSweepLineReference(&(this->sweepLinePosition));

	//!
	this->beachLine = new openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine(this->nodeInquirer);
	this->beachLine->setPoints(this->sitePoint);
	//this->beachLine->setHalfEdges(this->halfEdges);
	this->beachLine->setQueueEvent(this->eventQueue);
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
		this->sitePoint->push_back(site);
	}
}

void openLife::procedure::diagram::voronoi::FortuneAlgorithm::buildDiagram()
{
	//!create event from sites
	if(!this->sitePoint || !this->sitePoint->size()) return;
	this->eventQueue->addSiteEvent(*(this->sitePoint));

	//!
	if(!this->vertices)
	{
		this->vertices = new std::vector<beachline::VertexPtr>();
	}

	//! initialize vector of halfedges for faces
	this->faces->resize(this->sitePoint->size(), nullptr);

	//!beachLine last settings
	this->beachLine->setSitePointLimitValues(
			0, this->dimension.width,
			0, this->dimension.height);

	// process events
	while (!this->eventQueue->isEmpty())
	{
		EventPtr currentEvent = this->eventQueue->getNextEvent();
		this->sweepLinePosition = currentEvent->point.y;// set position of a sweepline
		printf("\n\n-------------------------------------------------------------------------------------------");
		printf("\n\nmove sweepLine to [%i](%.1f, %.1f):", currentEvent->index, currentEvent->point.x, currentEvent->point.y);

		if (currentEvent->type == Event::SITE)// handle site event
		{
			Point2D newPoint = currentEvent->point;

			this->beachLine->addArc(currentEvent->index, newPoint);
			while(EventPtr newEvent = this->beachLine->getNewEvent())
			{
				this->eventQueue->get()->push(newEvent);
			}
		}
		else if (currentEvent->type == Event::CIRCLE)
		{
			Debug::inquire(currentEvent)->printInfo("\n\thandle circle event");
			// handle circle event
			beachline::BLNodePtr arc = currentEvent->arc;
			Point2D center = currentEvent->center;
			this->beachLine->moveToEdgeEndPoint(currentEvent->point, arc, center);


			/**********************************************************************************************************/

			std::pair<beachline::BLNodePtr, beachline::BLNodePtr> breakpoints;
			breakpoints = this->beachLine->breakpoints;

			if(!this->beachLine->isValidBreakPoints(breakpoints))continue;

			/**********************************************************************************************************/


			// create a new vertex and insert into doubly-connected edge list
			beachline::VertexPtr vertex = std::make_shared<beachline::Vertex>(currentEvent->center);
			printf("\n\tcreate vertext {id:? coord(%f, %f)}",vertex->point.x, vertex->point.y);

			beachline::HalfEdgePtr h_first = breakpoints.first->edge;
			beachline::HalfEdgePtr h_second = breakpoints.second->edge;

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

			//!store pointers to the next and previous leaves
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
			this->beachLine->firstArc = beachline::remove(arc);

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
			h_first->vertex = vertex;
			h_second->vertex = vertex;
			twin_nodes.second->vertex = vertex;
			vertex->edge = h_second;

			this->halfEdges->push_back(twin_nodes.first);
			this->halfEdges->push_back(twin_nodes.second);

			// check new circle events
			if (prev_leaf != nullptr && next_leaf != nullptr) {
				EventPtr circle_event = checkCircleEvent(prev_leaf->prev, prev_leaf, next_leaf, *this->sitePoint, this->sweepLinePosition);
				if (circle_event != nullptr) {
					this->eventQueue->get()->push(circle_event);
				}
				circle_event = checkCircleEvent(prev_leaf, next_leaf, next_leaf->next, *this->sitePoint, this->sweepLinePosition);
				if (circle_event != nullptr) {
					this->eventQueue->get()->push(circle_event);
				}
			}
			/**********************************************************************************************************
			//!store newly generated vertices
			for(int i=0; i<this->beachLine->getVertices()->size(); i++)
			{
				this->vertices->push_back(this->beachLine->getVertices()->at(i));
			}


			//!store newly generated halfEdges
			for(int i=0; i<this->beachLine->getHalfEdges()->size(); i++)
			{
				this->halfEdges->push_back(this->beachLine->getHalfEdges()->at(i));
			}

			while(EventPtr newEvent = this->beachLine->getNewEvent())
			{
				this->eventQueue->get()->push(newEvent);
			}
			/**************************************************************************************************************/

		}
		printf("\n\t=====>number Events left: %lu", this->eventQueue->get()->size());
	}

	// Fill edges corresponding to faces
	for (size_t i = 0; i < this->halfEdges->size(); ++i) {
		beachline::HalfEdgePtr he = this->halfEdges->at(i);
		if (he->prev == nullptr || this->faces->at(he->l_index) == nullptr)
		{
			this->faces->at(he->l_index) = he;
		}
	}
}

NodeInquirer* openLife::procedure::diagram::voronoi::FortuneAlgorithm::inquire(beachline::BLNodePtr node)
{
	this->nodeInquirer->setSubject(node);
	return this->nodeInquirer;
}

void openLife::procedure::diagram::voronoi::FortuneAlgorithm::setOutputDataStruct(VoronoiDiagram *diagram)
{
	this->vertices = &(diagram->vertices);
	this->dimension.width = diagram->dimension.width;
	this->dimension.height = diagram->dimension.height;
}

void openLife::procedure::diagram::voronoi::FortuneAlgorithm::setSurfaceDimension(double width, double height)
{
	this->dimension.width = width;
	this->dimension.height = height;
}
