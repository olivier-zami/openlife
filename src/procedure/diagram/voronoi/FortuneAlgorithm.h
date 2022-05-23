//
// Created by olivier on 10/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI

#include "VoronoiDiagram.h"

#include <vector>
#include "fortuneAlgorithm/Types/Point2D.h"
#include "fortuneAlgorithm/objectType/Node.hpp"
#include "fortuneAlgorithm/objectType/NodeInquirer.h"

using namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm;

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
			void buildDiagram();
			NodeInquirer* inquire(beachline::BLNodePtr node);
			void setOutputDataStruct(VoronoiDiagram* diagram);
			void setSurfaceDimension(double width, double height);


		private:
			static openLife::procedure::diagram::voronoi::FortuneAlgorithm* instance;

			struct{
				double width;
				double height;
			}dimension;
			std::vector<beachline::HalfEdgePtr>* faces;
			std::vector<beachline::HalfEdgePtr>* halfEdges;
			NodeInquirer* nodeInquirer;
			size_t nbrSite;
			std::vector<Point2D>* sitePoint;
			std::vector<beachline::VertexPtr>* vertices;

		public: //TODO: change to private afterward
			double sweepLinePosition;
			openLife::procedure::diagram::voronoi::fortuneAlgorithm::BeachLine* beachLine;
			openLife::procedure::diagram::voronoi::fortuneAlgorithm::EventQueue* eventQueue;
	};
}


#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI
