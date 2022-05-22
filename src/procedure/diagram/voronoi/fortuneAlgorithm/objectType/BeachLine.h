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
namespace dataType = openLife::procedure::diagram::voronoi::fortuneAlgorithm::dataType;

namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm
{
	class BeachLine
	{
		public:
			BeachLine(NodeInquirer* nodeInquirer);
			~BeachLine();

			void addArc(int idSite, Point2D newPoint);
			void flushEvents();
			void moveToEdgeEndPoint(Point2D newPoint, beachline::BLNodePtr arc, Point2D center);
			void setSitePointLimitValues(double xMin, double xMax, double yMin, double yMax);
			void setPoints(std::vector<Point2D>* points);
			void setHalfEdges(std::vector<beachline::HalfEdgePtr>* halfEdges);
			void setQueueEvent(openLife::procedure::diagram::voronoi::fortuneAlgorithm::EventQueue* eventQueue);

		//private:

			BLNodePtr createNode(
					int type,
					int index,
					Point2D point,
					int ind1,
					int ind2);
			BLNodePtr createTree(int index, int index_behind, double *sweepline,
							const std::vector<Point2D> *points,
							std::vector<HalfEdgePtr> &edges);

			BLNodePtr createSimpleTree(int index, int index_behind, double *sweepline,
									  const std::vector<Point2D> *points,
									  std::vector<HalfEdgePtr> &edges);

			BLNodePtr find(BLNodePtr root, int idSite, Point2D point);
			std::pair<BLNodePtr, BLNodePtr> getBreakpoints(BLNodePtr leaf);
			beachline::VertexPtr getEdge();
			EventPtr getNewEvent();
			double getSweepLineEquidistantPointFromFoci(BLNodePtr node);//TODO:rename to getSomething getSweepLineEquidistantPointFromFoci();
			std::vector<Point2D> getParabolasIntersections(const Point2D &focal1, const Point2D &focal2);
			std::vector<beachline::HalfEdgePtr>* getHalfEdges();
			std::queue<beachline::VertexPtr>* getVertices();
			NodeInquirer* inquire(beachline::BLNodePtr node);
			bool isEmpty();
			bool isValidBreakPoints(std::pair<beachline::BLNodePtr, beachline::BLNodePtr> breakpoints);
			BLNodePtr replace(BLNodePtr node, BLNodePtr new_node);


			std::vector<EventPtr>* circleEvent;
			beachline::BLNodePtr firstArc;
			std::vector<beachline::HalfEdgePtr>* halfEdges;
			std::queue<EventPtr>* newEvent;
			NodeInquirer* nodeInquirer;
			std::vector<Point2D>* sitePoint;
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
			std::queue<beachline::VertexPtr>* vertex;
			openLife::procedure::diagram::voronoi::fortuneAlgorithm::EventQueue* eventQueue;

			std::pair<beachline::BLNodePtr, beachline::BLNodePtr> breakpoints;
	};
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_BEACHLINE_H
