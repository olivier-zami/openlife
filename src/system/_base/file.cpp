//
// Created by olivier on 13/08/2021.
//

#include "file.h"

#include <fstream>
#include <cstdio>
#include <vector>

int openLife::system::File::create(const char *filename)
{
	int success;
	if(FILE *file = fopen(filename, "w"))
	{
		fclose(file);
		success = true;
	}
	else success = false;
	return success;
}

int openLife::system::File::exists(const char *filename)
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

int openLife::system::File::remove(const char *filename)
{
	return ::remove(filename);
}

std::vector<std::string> openLife::system::File::find(const char *filename)
{
	std::vector<std::string> found;
	return found;
}

std::string openLife::system::File::fullName(const char *filename)
{
	std::string fullname;
	return fullname;
}
