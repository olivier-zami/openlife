//
// Created by olivier on 11/05/2022.
//

#include "EventQueue.h"

/**
 *
 * @note : create a priority queue
 */
openLife::procedure::diagram::voronoi::fortuneAlgorithm::EventQueue::EventQueue()
{
	this->pq = new std::priority_queue<EventPtr, std::vector<EventPtr>, EventPtrComparator>();
}

openLife::procedure::diagram::voronoi::fortuneAlgorithm::EventQueue::~EventQueue() {}

/**
 *
 * @param randomPoint
 * @note initialize it with all site events
 */
void openLife::procedure::diagram::voronoi::fortuneAlgorithm::EventQueue::addSiteEvent(std::vector <Point2D> randomPoint)
{
	for (size_t i = 0; i < randomPoint.size(); ++i)
	{
		this->pq->push(std::make_shared<Event>(static_cast<int>(i), Event::SITE, randomPoint[i]));
	}
}

/**
 *
 * @return
 * @note extract new event from the queue
 */
EventPtr openLife::procedure::diagram::voronoi::fortuneAlgorithm::EventQueue::getNextEvent()
{
	EventPtr event = this->pq->top();
	this->pq->pop();
	return event;
}

std::priority_queue<EventPtr, std::vector<EventPtr>, EventPtrComparator> * openLife::procedure::diagram::voronoi::fortuneAlgorithm::EventQueue::get()
{
	return this->pq;
}

bool openLife::procedure::diagram::voronoi::fortuneAlgorithm::EventQueue::isEmpty()
{
	return this->pq->empty();
}