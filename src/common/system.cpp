//
// Created by olivier on 10/08/2021.
//

#include "system.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include "src/common/system/time.h"

void openLife::system::notice(const char *message)
{
	std::string date;
	date = openLife::system::Time::getCurrentDate();
	std::cout <<"\n"<< date <<" => notice : " << message <<"";
}

void openLife::system::notice(std::string message)
{
	std::string date;
	date = openLife::system::Time::getCurrentDate();
	std::cout <<"\n"<< date << " => notice : "<< message.c_str();
}

int openLife::system::isFileWritable()
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
