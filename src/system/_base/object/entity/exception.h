//
// Created by olivier on 17/08/2021.
//

#ifndef ONELIFETEST_SYSTEM_OBJECT_ENTITY_EXCEPTION_H
#define ONELIFETEST_SYSTEM_OBJECT_ENTITY_EXCEPTION_H

#include <string>

namespace openLife::system::object::entity
{
	class Exception
	{
		public:
			Exception(const char* message = nullptr);
			~Exception();

			openLife::system::object::entity::Exception* operator()(const char* message);

			std::string getMessage();

		private:
			std::string message;
	};
}

#endif //ONELIFETEST_SYSTEM_OBJECT_ENTITY_EXCEPTION_H
