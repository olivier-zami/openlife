//
// Created by olivier on 15/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATYPE_BEACHLINE_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATYPE_BEACHLINE_H

#include "../Types/Point2D.h"

namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm::dataType::beachLine
{
	typedef struct stc_node{
		unsigned int id;
		Point2D point;
		struct{
			double center;
			struct{
				double min;
				double max;
			}interval;
		}position;
		struct{
			unsigned int position;
			struct stc_node* previous;
			struct stc_node* next;
		}frontLineSection;
	}Node;

	typedef struct{
		int step;
		Node* node;
	}NodeProcess;

}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_DATYPE_BEACHLINE_H
