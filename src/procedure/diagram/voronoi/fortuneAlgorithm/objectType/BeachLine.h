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

			EventPtr getNewEvent();
			void moveToEdgeEndPoint(Point2D newPoint, beachline::BLNodePtr arc, Point2D center);
			void moveToSitePoint(int idSite, Point2D newPoint);
			void setSitePointLimitValues(double xMin, double xMax, double yMin, double yMax);
			void setPoints(std::vector<Point2D>* points);
			void setHalfEdges(std::vector<beachline::HalfEdgePtr>* halfEdges);
			void setQueueEvent(openLife::procedure::diagram::voronoi::fortuneAlgorithm::EventQueue* eventQueue);

			beachline::VertexPtr vertex;

		private:
			EventPtr checkCircleEvent(BLNodePtr n1, BLNodePtr n2, BLNodePtr n3,
									  const std::vector<Point2D> &points);
			BLNodePtr createNode(
					FA::dataType::beachLine::NodeType type,
					FA::dataType::beachLine::Site currentSite,
					int ind1,
					int ind2);
			BLNodePtr createTree(int index, int index_behind, double *sweepline,
							const std::vector<Point2D> *points,
							std::vector<HalfEdgePtr> &edges);

			BLNodePtr createSimpleTree(int index, int index_behind, double *sweepline,
									  const std::vector<Point2D> *points,
									  std::vector<HalfEdgePtr> &edges);
			FA::dataType::beachLine::Site createSitePoint(Point2D point);
			BLNodePtr find(BLNodePtr root, int idSite, Point2D point);
			bool findCircleCenter(const Point2D &p1, const Point2D &p2, const Point2D &p3, Point2D &center);
			std::pair<BLNodePtr, BLNodePtr> getBreakpoints(BLNodePtr leaf);
			beachline::VertexPtr getEdge();

			double getSweepLineEquidistantPointFromFoci(BLNodePtr node);//TODO:rename to getSomething getSweepLineEquidistantPointFromFoci();
			std::vector<Point2D> getParabolasIntersections(const Point2D &focal1, const Point2D &focal2);
			std::vector<beachline::HalfEdgePtr>* getHalfEdges();
			NodeInquirer* inquire(beachline::BLNodePtr node);
			bool isEmpty();
			bool isValidBreakPoints(std::pair<beachline::BLNodePtr, beachline::BLNodePtr> breakpoints);
			BLNodePtr replace(BLNodePtr node, BLNodePtr new_node);


			std::vector<EventPtr>* circleEvent;
			beachline::BLNodePtr firstArc;
			std::vector<beachline::HalfEdgePtr>* halfEdges;
			unsigned int idxSitePoint;
			std::queue<EventPtr>* newEvent;
			NodeInquirer* nodeInquirer;
			std::vector<Point2D>* sitePoint;
			std::vector<FA::dataType::beachLine::Site>* sitePoint1;
			struct{
				struct{
					double min;
					double max;
				}x;
				struct{
					double min;
					double max;
				}y;
			}sitePointLimit;
			double sweepLinePosition;
			//std::queue<beachline::VertexPtr>* vertex;
			openLife::procedure::diagram::voronoi::fortuneAlgorithm::EventQueue* eventQueue;
	};
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_BEACHLINE_H
