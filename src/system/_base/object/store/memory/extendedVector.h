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
	template<typename T> class ExtendedVector /*: public std:vector*///TODO: check for SimpleVector
	{
		public:
			ExtendedVector();
			~ExtendedVector();

			void reserve(int size);

			void put(int idx, int value);
			int get(int idx);

		private:
			std::vector<T> element;
			openLife::system::type::Value2D_U32 size;
	};
}

template<typename T> openLife::system::object::store::memory::ExtendedVector<T>::ExtendedVector()
{

	//this->element.reserve(size);
	/*
	 va_list ap;
    int j;
    double tot = 0;
    va_start(ap, count); //Requires the last fixed parameter (to get the address)
    for(j=0; j<count; j++)
        tot+=va_arg(ap, double); //Requires the type to cast to. Increments ap to the next argument.
    va_end(ap);
    return tot/count;
	 */
}

template<typename T> openLife::system::object::store::memory::ExtendedVector<T>::~ExtendedVector() {}

template<typename T> void openLife::system::object::store::memory::ExtendedVector<T>::reserve(int size)
{
	std::cout << "\nAllocation du vecteur : " << size;
}

template<typename T> void openLife::system::object::store::memory::ExtendedVector<T>::put(int idx, int value) {}

template<typename T> int openLife::system::object::store::memory::ExtendedVector<T>::get(int idx)
{
	return 0;
}

#endif //OPENLIFE_RANDOMACCESS_H
