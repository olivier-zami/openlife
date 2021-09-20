//
// Created by olivier on 07/08/2021.
//

#ifndef OPENLIFE_RANDOMACCESS_H
#define OPENLIFE_RANDOMACCESS_H

#include <cstdarg>
#include <vector>
#include <iostream>
#include "src/system/_base/type.h"

namespace openLife::system::object::store::memory
{
	template<typename T> class ExtendedVector2D : public std::vector<T>//TODO: check for SimpleVector
	{
		public:
			ExtendedVector2D();
			ExtendedVector2D(unsigned int x, unsigned int y);
			~ExtendedVector2D();

			size_t capacity();
			void reserve(unsigned int x, unsigned int y);

			void setDefaultValue(T defaultValue);
			void reset();
			openLife::system::type::Value2D_U32 getSize();
			unsigned int getTotalSize();

			void set(openLife::system::type::Value2D_U32 coord, T value);
			T get();
			T get(int idx);
			T get(unsigned int x, unsigned int y);

		private:
			std::vector<T> vector;
			T defaultValue;
			openLife::system::type::Value2D_U32 size;
			unsigned int totalSize = 0;
			unsigned int idx;
	};
}

template<typename T> openLife::system::object::store::memory::ExtendedVector2D<T>::ExtendedVector2D(){}

template<typename T> openLife::system::object::store::memory::ExtendedVector2D<T>::ExtendedVector2D(unsigned int x, unsigned int y)
{
	this->reserve(x, y);
}

template<typename T> openLife::system::object::store::memory::ExtendedVector2D<T>::~ExtendedVector2D() {}

template<typename T> size_t openLife::system::object::store::memory::ExtendedVector2D<T>::capacity()
{
	return this->vector.capacity();
}

template<typename T> void openLife::system::object::store::memory::ExtendedVector2D<T>::reserve(unsigned int x, unsigned int y)
{
	this->size.x = x;
	this->size.y = y;
	this->totalSize = this->size.x * this->size.y;
	this->vector.reserve(this->totalSize);
}

template<typename T> void openLife::system::object::store::memory::ExtendedVector2D<T>::setDefaultValue(T defaultValue)
{
	this->defaultValue = defaultValue;
}

template<typename T> void openLife::system::object::store::memory::ExtendedVector2D<T>::reset()
{
	for(unsigned int i=0; i<this->totalSize; i++)
	{
		this->vector[i] = this->defaultValue;
	}
}

template<typename T> openLife::system::type::Value2D_U32 openLife::system::object::store::memory::ExtendedVector2D<T>::getSize()
{
	return this->size;
}

template<typename T> unsigned int openLife::system::object::store::memory::ExtendedVector2D<T>::getTotalSize()
{
	return this->totalSize;
}

template<typename T> void openLife::system::object::store::memory::ExtendedVector2D<T>::set(openLife::system::type::Value2D_U32 coord, T value)
{
	if(coord.x >= this->size.x);//TODO throw exception
	if(coord.y >= this->size.y);//TODO throw exception
	unsigned int idx = coord.x + this->size.x * coord.y;
	this->vector[idx] = value;
}

template<typename T> T openLife::system::object::store::memory::ExtendedVector2D<T>::get()
{
	return this->vector[this->idx];
}

template<typename T> T openLife::system::object::store::memory::ExtendedVector2D<T>::get(int idx)
{
	return this->vector[idx];
}

template<typename T> T openLife::system::object::store::memory::ExtendedVector2D<T>::get(unsigned int x, unsigned int y)
{
	if(x >= this->size.x);//TODO throw exception
	if(y >= this->size.y);//TODO throw exception
	unsigned int idx = x + this->size.x * y;
	return this->vector[idx];
}

#endif //OPENLIFE_RANDOMACCESS_H
