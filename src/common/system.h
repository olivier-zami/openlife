//
// Created by olivier on 10/08/2021.
//

#ifndef OPENLIFE_SYSTEM_H
#define OPENLIFE_SYSTEM_H

#include <string>

namespace openLife::system
{
	void notice(const char *message);
	void notice(std::string message);

	void warning(const char *message);
	void error(const char *message);

	int isFileWritable();
}

#endif //OPENLIFE_SYSTEM_H
