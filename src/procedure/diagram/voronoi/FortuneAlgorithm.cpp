//
// Created by olivier on 10/05/2022.
//

#include "FortuneAlgorithm.h"

#include <queue>

#include "fortuneAlgorithm/dataType/Event.h"
#include "fortuneAlgorithm/Debug.h"
#include "fortuneAlgorithm/objectType/Node.hpp"
#include "fortuneAlgorithm/objectType/BeachLine.h"
#include "fortuneAlgorithm/objectType/EventQueue.h"

namespace FA = openLife::procedure::diagram::voronoi::fortuneAlgorithm;
namespace nsSingleton = openLife::procedure::diagram::voronoi;
using namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm;

openLife::procedure::diagram::voronoi::FortuneAlgorithm* openLife::procedure::diagram::voronoi::FortuneAlgorithm::instance = nullptr;


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
		this->eventQueue->addSiteEvent(i, site);
		this->nbrSite++;
	}
}

void openLife::procedure::diagram::voronoi::FortuneAlgorithm::buildDiagram()
{
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

			this->beachLine->moveToSitePoint(currentEvent->index, newPoint);
			while(EventPtr newEvent = this->beachLine->getNewEvent())
			{
				this->eventQueue->get()->push(newEvent);
			}
		}
		else if (currentEvent->type == Event::CIRCLE)// handle circle event
		{
			this->beachLine->moveToEdgeEndPoint(currentEvent->point, currentEvent->arc, currentEvent->center);

			//!create a new vertex and insert into doubly-connected edge list
			this->vertices->push_back(this->beachLine->vertex);// store vertex of Voronoi diagram

			while(EventPtr newEvent = this->beachLine->getNewEvent())
			{
				this->eventQueue->get()->push(newEvent);
			}
		}
		printf("\n\t=====>number Events left: %lu", this->eventQueue->get()->size());
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
