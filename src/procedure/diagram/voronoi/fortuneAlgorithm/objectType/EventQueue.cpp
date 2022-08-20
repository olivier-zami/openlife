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
void openLife::procedure::diagram::voronoi::fortuneAlgorithm::EventQueue::addSiteEvent(unsigned int index, Point2D randomPoint)
{
	this->pq->push(std::make_shared<Event>(static_cast<int>(index), Event::SITE, randomPoint));
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