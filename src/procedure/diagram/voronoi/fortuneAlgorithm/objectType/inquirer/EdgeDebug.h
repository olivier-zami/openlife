//
// Created by olivier on 27/05/2022.
//

#ifndef OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_INQUIRER_EDGEDEBUG_H
#define OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_INQUIRER_EDGEDEBUG_H

#include "../../dataType/Edge.h"

namespace FA = openLife::procedure::diagram::voronoi::fortuneAlgorithm;

namespace openLife::procedure::diagram::voronoi::fortuneAlgorithm::inquirer
{
	class EdgeDebug
	{
		public:
			EdgeDebug();
			~EdgeDebug();

			void setSubject(FA::dataType::Edge* edge);
			void printInfo(const char* label = nullptr);

		private:
			FA::dataType::Edge* subject;
	};
}

#endif //OPENLIFE_SRC_PROCEDURE_DIAGRAM_VORONOI_FORTUNEALGORITHM_OBJECTTYPE_INQUIRER_EDGEDEBUG_H
