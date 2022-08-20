//
// Created by olivier on 02/09/2021.
//

#include "point2D.h"

#include <iostream>
#include <vector>
#include <cmath>

std::vector<openLife::dataType::geometric::Point2D_32> openLife::system::process::container::point2D::getCoastalShape(
		openLife::system::process::container::point2D::coastalShapeSettings settings)
{
	std::cout << "\nSet landMass center : ("<<settings.center.x<<", "<<settings.center.y<<")";
	std::cout << "\nLandmass number : " << settings.initialPoint.number;
	std::cout << "\nProcess iteration : " << settings.process.iteration;

	unsigned int step = (1 << settings.process.iteration);
	unsigned int nbrPoint = settings.initialPoint.number * step;

	std::vector<openLife::dataType::geometric::Point2D_32> coastalPoint(nbrPoint);
	unsigned int ptr = step -1;
	for(float i = 0; i< 2*3.14159; i+=(2*3.14159/settings.initialPoint.number))
	{
		coastalPoint[ptr] = {(int32_t)((double)settings.center.x+sin(i)*90), (int32_t)((double)settings.center.y+cos(i)*90)};
		ptr += step;
	}


	std::cout << "\njump: " << step;
	std::cout << "\ncoastalPoint : ";
	for(unsigned int i = 0; i <coastalPoint.size(); i++)
	{
		std::cout << "\n["<<i<<"] : ("<<coastalPoint[i].x<<", "<<coastalPoint[i].y<<")";
	}


	for(step; step>=2; step /= 2)
	{
		for(unsigned int i = step-1; i<coastalPoint.size(); i+=step)
		{
			unsigned int idxPreviousPoint = i<step ? coastalPoint.size()-1 : i-step;
			float dx = (float)coastalPoint[i].x - (float)coastalPoint[idxPreviousPoint].x;
			float dy = (float)coastalPoint[i].y - (float)coastalPoint[idxPreviousPoint].y;
			float length = sqrt((dx*dx)+(dy*dy));
			openLife::dataType::geometric::Point2D_32 middlePoint = {
					(int32_t)((float)coastalPoint[idxPreviousPoint].x+(float)(dx/2)),
					(int32_t)((float)coastalPoint[idxPreviousPoint].y+(float)(dy/2))};
			float sin = dx/length;
			float cos = dy/length;


			unsigned int idxNewPoint = i-(step/2);
			//std::cout << "\n====> ("<< sin <<", "<< cos <<")";

			coastalPoint[idxNewPoint].x = middlePoint.x - 20*cos /*- (50*(int32_t)cos)*/;
			coastalPoint[idxNewPoint].y = middlePoint.y + 20*sin/*+ (50*(int32_t)sin)*/;

			std::cout << "\nInterpolation ";
			std::cout << "["<<idxPreviousPoint<<"]("<<coastalPoint[idxPreviousPoint].x<<", "<<coastalPoint[idxPreviousPoint].y<<") => ";
			std::cout << "["<<i<<"]("<<coastalPoint[i].x<<" ,"<<coastalPoint[i].y<<") ";
			std::cout << "| middle : (" << middlePoint.x <<", "<< middlePoint.y <<") ";
			//std::cout << "| "<<length<<"*("<<sin<<", "<<cos<<") = " << "("<<dx<<", "<<dy<<") ";
			std::cout << "| newPoint : ( " << coastalPoint[idxNewPoint].x << ", " << coastalPoint[idxNewPoint].y << ")";
		}
	}


	std::cout << "\nprocess ...\n";
	return coastalPoint;
}