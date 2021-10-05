//
// Created by olivier on 13/08/2021.
//

#ifndef ONELIFETEST_SYSTEM_FILE_H
#define ONELIFETEST_SYSTEM_FILE_H

#include <string>
#include <vector>

namespace openLife::system
{
	class File
	{
		public:
			static int create(const char* filename);
			static int remove(const char* filename);
			static bool find(const char* filename);
			static std::string fullName(const char* filename);
	};
}

#endif //ONELIFETEST_SYSTEM_FILE_H
