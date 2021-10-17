include /home/olivier/Projets/OpenLife/conf/makefile.conf

NAMESPACE =

DIR_EXECUTABLE = ${DIR_APP}/tools
DIR_MAIN = ${DIR_SRC}/tools
COMPILATION_FLAG = ${CFL_STRICT} ${CFL_SDL}
#app environment building
.PHONY: openLifeAdministrator openLifeServer

openLifeAdministrator:
	${CMD_COMPILATION} -o ${DIR_EXECUTABLE}/openLifeAdministrator -I${DIR_PROJECT} \
	${DIR_MAIN}/openLifeAdministrator.cpp \

openLifeServer:
	${CMD_COMPILATION} -o ${DIR_EXECUTABLE}/openLifeServer -I${DIR_PROJECT} \
	${DIR_MAIN}/openLifeServer.cpp \
	${DIR_PROJECT}/src/system/_base/object/entity/exception.cpp \
	${DIR_PROJECT}/src/server/channel/ipc.cpp
