//
// Created by olivier on 20/09/2021.
//

#ifndef OPENLIFE_OBJECT_ABSTRACT_SERVICE_H
#define OPENLIFE_OBJECT_ABSTRACT_SERVICE_H

#include <iostream>
#include <vector>

namespace openLife::system::object::abstract
{
	class Service
	{
		public:
			Service(){}
			~Service(){}

			//virtual void* getData(unsigned int idDataType) { return 0; }//TODO: check in test for use case

		protected:
			std::vector<void*> data;
	};
}
#endif //OPENLIFE_OBJECT_ABSTRACT_SERVICE_H
