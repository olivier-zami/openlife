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

EventPtr checkCircleEvent(bl::BLNodePtr n1, bl::BLNodePtr n2, bl::BLNodePtr n3,
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

			if(!this->beachLine->isEmpty())
			{
				this->beachLine->addArc(currentEvent->index, newPoint);
				for(int i=0; i<this->beachLine->getNewEvent()->size(); i++)
				{
					this->eventQueue->get()->push(this->beachLine->getNewEvent()->at(i));
				}
				this->beachLine->flushEvents();
			}
			else this->beachLine->addArc(currentEvent->index, newPoint);
		}
		else if (currentEvent->type == Event::CIRCLE)
		{
			Debug::inquire(currentEvent)->printInfo("\n\thandle circle event");
			// handle circle event
			beachline::BLNodePtr arc = currentEvent->arc;
			Point2D center = currentEvent->center;
			this->beachLine->moveToEdgeEndPoint(currentEvent->point, arc, center);

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


			printf("\n\t%lu events for insertion", this->beachLine->getNewEvent()->size());
			for(int i=0; i<this->beachLine->getNewEvent()->size(); i++)
			{
				this->eventQueue->get()->push(this->beachLine->getNewEvent()->at(i));
				printf("\n\t\tinsert Event ...");
			}
			this->beachLine->flushEvents();
		}
		printf("\n\t=====>number Events left: %lu", this->eventQueue->get()->size());
	}

	// Fill edges corresponding to faces
	for (size_t i = 0; i < this->halfEdges->size(); ++i) {
		bl::HalfEdgePtr he = this->halfEdges->at(i);
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
