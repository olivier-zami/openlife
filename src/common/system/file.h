//
// Created by olivier on 13/08/2021.
//

#ifndef ONELIFETEST_FILE_H
#define ONELIFETEST_FILE_H

#include <string>
#include <vector>

namespace openLife::system
{
	class File
	{
		public:
			static int remove(const char* filename);
			static int exists(const char* filename);
			static std::vector<std::string> find(const char* filename);
			static std::string absoluteName(const char* filename);
	};
}

#endif //ONELIFETEST_FILE_H
