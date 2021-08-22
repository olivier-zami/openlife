//
// Created by olivier on 21/08/2021.
//

#ifndef OPENLIFE_SYSTEM_OBJECT_PROCESS_SERVICE_H
#define OPENLIFE_SYSTEM_OBJECT_PROCESS_SERVICE_H

namespace openLife::system::object::process
{
	class Service
	{
		public:
			Service();
			~Service();

			static void setTypeId(unsigned int id);
			static unsigned int getTypeId();

			void setId(unsigned int id);
			unsigned int getId();

			void start();

		protected:
			unsigned int id;
			static unsigned int idType;
	};
}

#endif //OPENLIFE_SYSTEM_OBJECT_PROCESS_SERVICE_H
