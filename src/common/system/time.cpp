//
// Created by olivier on 10/08/2021.
//

#include "time.h"
#include <ctime>

std::string openLife::system::Time::getCurrentDate(const char* format)
{
	char date[100];
	time_t tick = std::time(0);
	if(!format) std::strftime(date, sizeof(date), "%Y/%m/%d %H:%M:%S", std::localtime(&tick));
	else std::strftime(date, sizeof(date), format, std::localtime(&tick));
	std::string currentDate(date);
	return currentDate;
}