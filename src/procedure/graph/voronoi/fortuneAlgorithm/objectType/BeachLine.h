//
// Created by olivier on 11/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_BEACHLINE_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_BEACHLINE_H

#include <queue>
#include <stack>
#include <vector>

#include "../dataType/beachLine.h"
#include "../dataType/Event.h"
#include "../dataType/HalfEdge.h"
#include "../dataType/Site.h"
#include "Node.hpp"
#include "NodeInquirer.h"
#include "EventQueue.h"

#define BREAKPOINTS_EPSILON 1.0e-5
#define CIRCLE_CENTER_EPSILON 1.0e-7

using namespace beachline;
namespace FA = openLife::procedure::diagram::voronoi::fortuneAlgorithm;

namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm
{
	class BeachLine
	{
		public:
			BeachLine(NodeInquirer* nodeInquirer);
			~BeachLine();

			std::vector<FA::dataType::EdgePtr>* getEdges();
			EventPtr getNewEvent();
			void moveToEdgeEndPoint(Point2D newPoint, beachline::BLNodePtr arc, Point2D center);
			void moveToSitePoint(Point2D newPoint);
			void setHalfEdges(std::vector<beachline::HalfEdgePtr>* halfEdges);

			beachline::VertexPtr vertex;

		//private:
			EventPtr checkCircleEvent(BLNodePtr n1, BLNodePtr n2, BLNodePtr n3);
			FA::dataType::EdgePtr createEdge(
					FA::dataType::Site* site1,
					FA::dataType::Site* site2);
			FA::dataType::EdgeEndPtr createEdgeEnd(Point2D point);
			BLNodePtr createNode(
					FA::dataType::beachLine::NodeType type,
					int ind1,
					int ind2);
			FA::dataType::SitePtr createSitePoint(Point2D point);
			BLNodePtr createTree(int index, int index_behind, std::vector<HalfEdgePtr> &edges);
			BLNodePtr createSimpleTree(int index, int index_behind, std::vector<HalfEdgePtr> &edges);
			bool findCircleCenter(const Point2D &p1, const Point2D &p2, const Point2D &p3, Point2D &center);
			std::pair<BLNodePtr, BLNodePtr> getBreakpoints(BLNodePtr leaf);
			FA::dataType::Edge* getEdge(FA::dataType::Site site1, FA::dataType::Site site2);
			FA::dataType::EdgeEndPtr getEdgeEnd(Point2D point);
			BLNodePtr getFirstNeighborSiteNode(FA::dataType::SitePtr localSite);
			std::vector<beachline::HalfEdgePtr>* getHalfEdges();
			FA::dataType::Edge* getOrCreateEdge(
					FA::dataType::Site* site1,
					FA::dataType::Site* site2);
			FA::dataType::EdgeEndPtr getOrCreateEdgeEnd(Point2D point);
			unsigned int getParabolasIntersectionNumber(const Point2D &focal1, const Point2D &focal2);
			std::vector<Point2D> getParabolasIntersections(const Point2D &focal1, const Point2D &focal2);
			double getSweepLineEquidistantPointFromFoci(BLNodePtr node);//TODO:rename to getSomething getSweepLineEquidistantPointFromFoci();
			FA::NodeInquirer* inquire(BLNodePtr node);
			bool isEmpty();
			bool isValidBreakPoints(std::pair<beachline::BLNodePtr, beachline::BLNodePtr> breakpoints);
			void replace(BLNodePtr node, BLNodePtr new_node);

			std::vector<FA::dataType::EdgePtr>* cellEdge;
			std::vector<FA::dataType::EdgeEndPtr>* cellEdgeEnd;
			std::vector<FA::dataType::SitePtr>* cellSite;
			std::vector<EventPtr>* circleEvent;
			beachline::BLNodePtr firstArc;
			std::vector<beachline::HalfEdgePtr>* halfEdges;
			std::queue<EventPtr>* newEvent;
			NodeInquirer* nodeInquirer;
			double sweepLinePosition;
			//std::queue<beachline::VertexPtr>* vertex;
			openLife::procedure::diagram::voronoi::fortuneAlgorithm::EventQueue* eventQueue;
	};
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_BEACHLINE_H
