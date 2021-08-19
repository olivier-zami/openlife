//
// Created by olivier on 17/08/2021.
//

#include "exception.h"

openLife::system::object::entity::Exception::Exception(const char *message)
{
	if(message != nullptr)
	{
		this->message = message;
	}
}

openLife::system::object::entity::Exception::~Exception() {}

openLife::system::object::entity::Exception *openLife::system::object::entity::Exception::operator()(const char *message)
{
	this->message = message;
	return this;
}

std::string openLife::system::object::entity::Exception::getMessage()
{
	return this->message;
}