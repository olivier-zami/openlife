//
// Created by olivier on 17/08/2021.
//

#include "exception.h"

openLife::system::object::entity::Exception::Exception() {}
openLife::system::object::entity::Exception::~Exception() {}

openLife::system::object::entity::Exception *openLife::system::object::entity::Exception::operator()(const char *message)
{
	this->message = message;
}

std::string openLife::system::object::entity::Exception::getMessage()
{
	return this->message;
}