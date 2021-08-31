//
// Created by olivier on 30/08/2021.
//

#include "nlohmann.h"

#include <fstream>

nlohmann::json openLife::system::nlohmann::getJsonFromFile(const char *filename)
{
	char *buffer;
	::nlohmann::json json;
	std::ifstream fileReader (filename);
	if(fileReader)
	{
		fileReader.seekg (0, fileReader.end);
		int length = fileReader.tellg();
		fileReader.seekg (0, fileReader.beg);
		buffer = new char [length];
		fileReader.read (buffer,length);
		fileReader.close();
		json = ::nlohmann::json::parse(buffer);
		delete[] buffer;
	}
	return json;
}