//
// Created by olivier on 10/08/2021.
//

#include "system.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include "src/common/system/time.h"

void common::system::notice(const char *message)
{
	std::string date;
	date = common::system::Time::getCurrentDate();
	std::cout <<"\n"<< date <<" => notice : " << message <<"";
}

void common::system::notice(std::string message)
{
	std::string date;
	date = common::system::Time::getCurrentDate();
	std::cout <<"\n"<< date << " => notice : "<< message.c_str();
}

int common::system::isFileWritable()
{
	const char *testFileName = "testReadOnly.txt";

	FILE *testFile = fopen( testFileName, "w" );

	if( testFile != NULL ) {

		fclose( testFile );
		remove( testFileName );
		return true;
	}
	return false;
}
