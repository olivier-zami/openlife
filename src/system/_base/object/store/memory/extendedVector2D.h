//
// Created by olivier on 07/08/2021.
//

#ifndef OPENLIFE_RANDOMACCESS_H
#define OPENLIFE_RANDOMACCESS_H

#include <cstdarg>
#include <vector>
#include <iostream>

namespace openLife::system::object::store::memory
{
	template<typename T> class ExtendedVector2D : public std::vector<T>//TODO: check for SimpleVector
	{
		public:
			ExtendedVector2D(unsigned int x, unsigned int y);
			~ExtendedVector2D();

			size_t capacity();

			void setDefaultValue(T defaultValue);
			void reset();

			openLife::system::object::store::memory::ExtendedVector2D<T>* select(unsigned int x, unsigned int y);
			void set(T value);
			T get();
			T get(int idx);

		private:
			std::vector<T> vector;
			T defaultValue;
			struct{
				unsigned int x;
				unsigned int y;
			}size;
			unsigned int totalSize;
			unsigned int idx;
	};
}

template<typename T> openLife::system::object::store::memory::ExtendedVector2D<T>::ExtendedVector2D(unsigned int x, unsigned int y)
{
	this->size.x = x;
	this->size.y = y;
	this->totalSize = this->size.x * this->size.y;
	this->vector.reserve(this->totalSize); //dom't work for some reasons
}

template<typename T> openLife::system::object::store::memory::ExtendedVector2D<T>::~ExtendedVector2D() {}

template<typename T> size_t openLife::system::object::store::memory::ExtendedVector2D<T>::capacity()
{
	return this->vector.capacity();
}

/*
template<typename T> void openLife::system::object::store::memory::ExtendedVector2D<T>::reserve(int size)
{
	std::cout << "\nAllocation du vecteur : " << size;
}
*/

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

template<typename T> openLife::system::object::store::memory::ExtendedVector2D<T>* openLife::system::object::store::memory::ExtendedVector2D<T>::select(
		unsigned int x,
		unsigned int y)
{
	if(x >= this->size.x);//TODO throw exception
	if(y >= this->size.y);//TODO throw exception
	this->idx = x + this->size.x * y;
	return this;
}

template<typename T> void openLife::system::object::store::memory::ExtendedVector2D<T>::set(T value)
{
	this->vector[this->idx] = value;
}

template<typename T> T openLife::system::object::store::memory::ExtendedVector2D<T>::get()
{
	return this->vector[this->idx];
}

template<typename T> T openLife::system::object::store::memory::ExtendedVector2D<T>::get(int idx)
{
	return this->vector[idx];
}

#endif //OPENLIFE_RANDOMACCESS_H
