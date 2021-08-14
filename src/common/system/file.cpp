//
// Created by olivier on 13/08/2021.
//

#include "file.h"

#include <fstream>
#include <cstdio>
#include <vector>

int common::system::File::exists(const char *filename)
{
	int exists;
	if(FILE *file = fopen(filename, "r"))
	{
		fclose(file);
		exists = true;
	}
	else exists = false;
	return exists;
}

int common::system::File::remove(const char *filename)
{
	return ::remove(filename);
}

std::vector<std::string> common::system::File::find(const char *filename)
{
	std::vector<std::string> found;
	return found;
}

std::string common::system::File::absoluteName(const char *filename)
{
	std::string fullname;
	return fullname;
}
