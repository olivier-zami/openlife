//
// Created by olivier on 21/08/2021.
//

#include "service.h"

unsigned int openLife::system::object::process::Service::idType = 0;

openLife::system::object::process::Service::Service(){}

openLife::system::object::process::Service::~Service(){}

void openLife::system::object::process::Service::setTypeId(unsigned int id)
{
	openLife::system::object::process::Service::idType = id;
}

unsigned int openLife::system::object::process::Service::getTypeId()
{
	return openLife::system::object::process::Service::idType;
}

void openLife::system::object::process::Service::setId(unsigned int id)
{
	this->id = id;
}

unsigned int openLife::system::object::process::Service::getId()
{
	return this->id;
}

void openLife::system::object::process::Service::start()
{

}