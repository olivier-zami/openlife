//
// Created by olivier on 17/10/2021.
//

#ifndef OPENLIFE_APPLICATION_H
#define OPENLIFE_APPLICATION_H

namespace openLife::system::object::abstract
{
	class Application
	{
		public:
			Application();
			~Application();

			void start();

		protected:
			void* state;
	};
}

#endif //OPENLIFE_APPLICATION_H
