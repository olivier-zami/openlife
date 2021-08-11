//
// Created by olivier on 10/08/2021.
//

#ifndef ONELIFETEST_ENVIRONMENT_H
#define ONELIFETEST_ENVIRONMENT_H

namespace common::system
{
	void notice(const char *message);
	void warning(const char *message);
	void error(const char *message);

	int isFileWritable();
}

#endif //ONELIFETEST_ENVIRONMENT_H
