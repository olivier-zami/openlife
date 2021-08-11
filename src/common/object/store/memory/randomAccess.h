//
// Created by olivier on 07/08/2021.
//

#ifndef OPENLIFE_RANDOMACCESS_H
#define OPENLIFE_RANDOMACCESS_H

namespace common::object::store::memory
{
	class RandomAccess
	{
		public:
			RandomAccess();
			~RandomAccess();

			void put(int idx, int value);
			int get(int idx);

		private:
			int* buffer;
	};
}

#endif //OPENLIFE_RANDOMACCESS_H
