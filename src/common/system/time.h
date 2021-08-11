//
// Created by olivier on 10/08/2021.
//

#ifndef ONELIFETEST_TIME_H
#define ONELIFETEST_TIME_H

#include <string>

namespace common::system
{
	class Time
	{
		public:
			static std::string getCurrentDate(const char* format = nullptr);
	};
}

#endif //ONELIFETEST_TIME_H
