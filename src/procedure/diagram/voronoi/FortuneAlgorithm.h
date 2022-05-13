//
// Created by olivier on 10/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI

#include "VoronoiDiagram.h"

#include <vector>
#include "fortuneAlgorithm/Types/Point2D.h"
#include "fortuneAlgorithm/dataType/Event.h"

#define BREAKPOINTS_EPSILON 1.0e-5
#define CIRCLE_CENTER_EPSILON 1.0e-7

namespace openLife::procedure::diagram::voronoi
{
	namespace fortuneAlgorithm
	{
		class BeachLine;
		class EventQueue;
	}

	class FortuneAlgorithm
	{
		public:
			static void generateDiagram(VoronoiDiagram* diagram, const std::vector<Point2D> &points);

			FortuneAlgorithm();
			~FortuneAlgorithm();

			void addSites(const std::vector<Point2D> &siteCoord);
			bool isEveryElementsSweep();
			void setOutputDataStruct(VoronoiDiagram* diagram);
			void sweepAll();
			void sweepNextElement();

		private:
			static openLife::procedure::diagram::voronoi::FortuneAlgorithm* instance;

			EventPtr currentEvent;
			std::vector<beachline::HalfEdgePtr>* faces;
			std::vector<beachline::HalfEdgePtr>* halfEdges;
			std::vector<beachline::VertexPtr>* vertices;
			std::vector<Point2D>* voronoiSite;

		public: //TODO: change to private afterward
			double sweepLinePosition;
			openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine* beachLine;
			openLife::procedure::diagram::voronoi::fortuneAlgorithm::EventQueue* eventQueue;
	};
}


#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI
