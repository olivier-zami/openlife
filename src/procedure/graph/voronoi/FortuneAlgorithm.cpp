//
// Created by olivier on 10/05/2022.
//

#include "FortuneAlgorithm.h"

#include <queue>
#include <stdexcept>

#include "../../../../src/dataType/geometric.h"
#include "../../../../src/procedure/math/point2D_F32.h"
#include "fortuneAlgorithm/dataType/Event.h"
#include "fortuneAlgorithm/Debug.h"
#include "fortuneAlgorithm/objectType/Node.hpp"
#include "fortuneAlgorithm/objectType/BeachLine.h"
#include "fortuneAlgorithm/objectType/EventQueue.h"

namespace FA = openLife::procedure::diagram::voronoi::fortuneAlgorithm;
namespace contextual = openLife::procedure::diagram::voronoi;
using namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm;

openLife::procedure::diagram::voronoi::FortuneAlgorithm* openLife::procedure::diagram::voronoi::FortuneAlgorithm::instance = nullptr;


/**
 *
 * @param diagram
 * @param points
 */
void openLife::procedure::diagram::voronoi::FortuneAlgorithm::generateDiagram(VoronoiDiagram* diagram, const std::vector<Point2D> &points)
{
	if (!contextual::FortuneAlgorithm::instance)
	{
		contextual::FortuneAlgorithm::instance = new openLife::procedure::diagram::voronoi::FortuneAlgorithm();
	}

	contextual::FortuneAlgorithm::instance->addSites(points);
	contextual::FortuneAlgorithm::instance->setOutputDataStruct(diagram);
	contextual::FortuneAlgorithm::instance->buildDiagram();
	diagram->edge = contextual::FortuneAlgorithm::instance->getEdges();
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
	this->nbrSite = 0;
	this->vertices = new std::vector<beachline::VertexPtr>();
	this->sitePoint = new std::vector<Point2D>();
	this->sweepLinePosition = 0L;//!current position of the sweep line

	//!
	this->nodeInquirer = new openLife::procedure::diagram::voronoi::fortuneAlgorithm::NodeInquirer();
	this->nodeInquirer->setSitePointsReference(this->sitePoint);
	this->nodeInquirer->setSweepLineReference(&(this->sweepLinePosition));

	//!
	this->beachLine = new openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine(this->nodeInquirer);
	//this->beachLine->setHalfEdges(this->halfEdges);
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
		this->eventQueue->addSiteEvent(i, site);
		this->nbrSite++;
	}
}

void openLife::procedure::diagram::voronoi::FortuneAlgorithm::buildDiagram()
{
	// process events
	while (!this->eventQueue->isEmpty())
	{
		EventPtr currentEvent = this->eventQueue->getNextEvent();
		this->sweepLinePosition = currentEvent->point.y;// set position of a sweepline
		//printf("\n\n-------------------------------------------------------------------------------------------");
		//printf("\n\nmove sweepLine to point[%i](%.1f, %.1f):", currentEvent->index, currentEvent->point.x, currentEvent->point.y);

		if (currentEvent->type == Event::SITE)// handle site event
		{
			Point2D newPoint = currentEvent->point;

			this->beachLine->moveToSitePoint(newPoint);
			while(EventPtr newEvent = this->beachLine->getNewEvent())
			{
				this->eventQueue->get()->push(newEvent);
			}
		}
		else if (currentEvent->type == Event::CIRCLE)// handle circle event
		{
			this->beachLine->moveToEdgeEndPoint(currentEvent->point, currentEvent->arc, currentEvent->center);

			while(EventPtr newEvent = this->beachLine->getNewEvent())
			{
				this->eventQueue->get()->push(newEvent);
			}
		}
	}

	//!complete unfinished edge
	printf("\n\nEdge list");
	for(size_t i =0; i<this->beachLine->getEdges()->size(); i++)
	{
		FA::dataType::Edge* edge = this->beachLine->getEdges()->at(i).get();

		//!clipping
		this->clipEdge(edge);

		if(!edge->end[0] || !edge->end[1])
		{
			this->completeEdge(this->beachLine->getEdges()->at(i).get());
		}
	}

	//!initialize vector of halfedges for faces
	this->faces->resize(this->nbrSite, nullptr);

	// Fill edges corresponding to faces
	for (size_t i = 0; i < this->halfEdges->size(); ++i) {
		beachline::HalfEdgePtr he = this->halfEdges->at(i);
		if (he->prev == nullptr || this->faces->at(he->l_index) == nullptr)
		{
			this->faces->at(he->l_index) = he;
		}
	}
}

void openLife::procedure::diagram::voronoi::FortuneAlgorithm::clipEdge(FA::dataType::Edge *edge)
{
	if(!edge->end[0] || !edge->end[1]) return; //edge must be complete

	double xMin = 0, xMax = 999, yMin = 0, yMax = 499;
	if( //out of shape TODO: check if edge entirely out of zone => min(edgeEnd.point.x)<xMax
			(edge->end[0]->point.x<xMin && edge->end[1]->point.x<xMin)
			|| (edge->end[0]->point.x>xMax && edge->end[1]->point.x>xMax)
			|| (edge->end[0]->point.y<yMin && edge->end[1]->point.y<yMin)
			|| (edge->end[0]->point.y>yMax && edge->end[1]->point.y>yMax)
	) return;

	struct {
		FA::dataType::EdgeEnd* start;
		FA::dataType::EdgeEnd* end;
	}clippedEdge;
	Point2D startPoint, endPoint;

	if((edge->end[0]->point.x != edge->end[1]->point.x))
	{
		if(edge->end[0]->point.x<edge->end[1]->point.x)
		{
			clippedEdge.start = edge->end[0];
			clippedEdge.end = edge->end[1];
			startPoint = edge->end[0]->point;
			endPoint = edge->end[1]->point;
		}
		else
		{
			clippedEdge.start = edge->end[1];
			clippedEdge.end = edge->end[0];
			startPoint = edge->end[1]->point;
			endPoint = edge->end[0]->point;
		}

		float a = (endPoint.y - startPoint.y) / (endPoint.x - startPoint.x);//TODO function getEdgeDirection

		if(startPoint.x<xMin)
		{
			startPoint.x = xMin;
			startPoint.y = (-a * (endPoint.x - startPoint.x)) + endPoint.y;
			this->createEdgeEnd(startPoint);
			clippedEdge.start = this->getEdgeEnd(startPoint);
			this->_debugPoint->push_back(endPoint);
			this->_debugPoint1->push_back(startPoint);//out
		}
		if(endPoint.x>xMax)//REM due the beachLine algorithm at least one edgeEnd must be in the surface
		{
			endPoint.x = xMax;
			endPoint.y = (a * (endPoint.x - startPoint.x)) + startPoint.y;
			this->createEdgeEnd(endPoint);
			clippedEdge.end = this->getEdgeEnd(endPoint);
			this->_debugPoint->push_back(startPoint);
			this->_debugPoint1->push_back(endPoint);//out
		}

		if(startPoint.y<yMin || startPoint.y>yMax)
		{
			startPoint.y = (startPoint.y<yMin) ? yMin : yMax;
			startPoint.x = ((endPoint.y - startPoint.y) / -a) + endPoint.x;
			//clippedEdge.start->point.x = xStart;
			//clippedEdge.start->point.y = yStart;
			this->_debugPoint->push_back(clippedEdge.end->point);
			this->_debugPoint1->push_back(startPoint);//out
		}
		if(endPoint.y<yMin || endPoint.y>yMax)
		{
			endPoint.y = (endPoint.y<yMin) ? yMin : yMax;
			endPoint.x = ((endPoint.y - startPoint.y) / a) + startPoint.x;
			//clippedEdge.end->point.x = xEnd;
			//clippedEdge.end->point.y = yEnd;
			this->_debugPoint->push_back(startPoint);
			this->_debugPoint1->push_back(endPoint);//out
		}
	}
	else if(edge->end[0]->point.y < edge->end[1]->point.y)
	{
		clippedEdge.start = edge->end[0];
		clippedEdge.end = edge->end[1];
	}
	else
	{
		clippedEdge.start = edge->end[1];
		clippedEdge.end = edge->end[0];
	}
}

void openLife::procedure::diagram::voronoi::FortuneAlgorithm::completeEdge(
		FA::dataType::Edge* edge)
{
	if(!edge || (!edge->end[0] && !edge->end[1]))
	{
		std::invalid_argument("Can not complete null edge or edge with no endEnd");
	}

	printf("\n\tcomplete edge [%i,%i] (%f, %f)", edge->site[0]->id, edge->site[1]->id, edge->end[0]->point.x, edge->end[0]->point.y);

	Point2D middlePoint;
	FA::dataType::EdgeEnd* validEdgeEnd;
	validEdgeEnd = edge->end[0] ? edge->end[0] : edge->end[1];

	if(validEdgeEnd->point.x < 0 || validEdgeEnd->point.x > 1000 || validEdgeEnd->point.y < 0 || validEdgeEnd->point.y > 500)
		return;

	//!save some special points for debug
	middlePoint.x = edge->site[0]->point.x + ((edge->site[1]->point.x - edge->site[0]->point.x)/2);
	middlePoint.y = edge->site[0]->point.y + ((edge->site[1]->point.y - edge->site[0]->point.y)/2);

	Point2D newEdgeEndPoint;
	if(validEdgeEnd->point.x != middlePoint.x)
	{
		float a = (validEdgeEnd->point.y - middlePoint.y) / (validEdgeEnd->point.x - middlePoint.x);//TODO function getEdgeDirection
		a = (validEdgeEnd->point.x > middlePoint.x) ? a : -a;

		if(validEdgeEnd->point.x>=500)//we suppose edge move to the right (end)
		{
			newEdgeEndPoint.x = 999;
			newEdgeEndPoint.y = (-a * (newEdgeEndPoint.x - validEdgeEnd->point.x))+ validEdgeEnd->point.y;
			if(newEdgeEndPoint.y < 0 || newEdgeEndPoint.y > 499)
			{
				newEdgeEndPoint.y = (newEdgeEndPoint.y < 0) ? 0 : 499;
				newEdgeEndPoint.x = ((newEdgeEndPoint.y - validEdgeEnd->point.y) / -a) + validEdgeEnd->point.x;
			}
		}
		else//we suppose edge move from the left (start)
		{
			newEdgeEndPoint.x = 0;
			newEdgeEndPoint.y = (-a * (validEdgeEnd->point.x - newEdgeEndPoint.x)) + validEdgeEnd->point.y;
			if(newEdgeEndPoint.y < 0 || newEdgeEndPoint.y > 499)
			{
				newEdgeEndPoint.y = (newEdgeEndPoint.y < 0) ? 0 : 499;
				newEdgeEndPoint.x = ((validEdgeEnd->point.y - newEdgeEndPoint.y) / - a) + validEdgeEnd->point.x;
			}
		}
	}
	else
	{
		printf("vertical edge : procedure niY"); exit(1);
	}
	this->createEdgeEnd(newEdgeEndPoint);
	edge->end[1] = this->getEdgeEnd(newEdgeEndPoint);
	this->_debugPoint->push_back(newEdgeEndPoint);
}

void openLife::procedure::diagram::voronoi::FortuneAlgorithm::createEdgeEnd(Point2D point)
{
	FA::dataType::EdgeEndPtr edgeEnd= this->beachLine->getOrCreateEdgeEnd(point);
	this->lastCreatedEdgeEndId = edgeEnd->id;
}

std::vector<FA::dataType::EdgePtr>* openLife::procedure::diagram::voronoi::FortuneAlgorithm::getEdges()
{
	return this->beachLine->getEdges();
}

FA::dataType::EdgeEnd* openLife::procedure::diagram::voronoi::FortuneAlgorithm::getEdgeEnd(Point2D point)
{
	return this->beachLine->getEdgeEnd(point).get();
}

NodeInquirer* openLife::procedure::diagram::voronoi::FortuneAlgorithm::inquire(beachline::BLNodePtr node)
{
	this->nodeInquirer->setSubject(node);
	return this->nodeInquirer;
}

void openLife::procedure::diagram::voronoi::FortuneAlgorithm::setOutputDataStruct(VoronoiDiagram *diagram)//TODO: rename handle(dataStruct)
{
	this->vertices = &(diagram->vertices);
	this->dimension.width = diagram->dimension.width;
	this->dimension.height = diagram->dimension.height;
	this->_debugPoint = &(diagram->debug.point);//new std::vector<Point2D>();
	this->_debugPoint1 = &(diagram->debug.point1);//new std::vector<Point2D>();
}

void openLife::procedure::diagram::voronoi::FortuneAlgorithm::setSurfaceDimension(double width, double height)
{
	this->dimension.width = width;
	this->dimension.height = height;
}
