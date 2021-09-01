include /home/olivier/Projets/OpenLife/conf/makefile.conf

NAMESPACE = tools

DIR_EXECUTABLE = ${DIR_APP}/tools
DIR_MAIN = ${DIR_SRC}/${NAMESPACE}
COMPILATION_FLAG = ${CFL_STRICT} ${CFL_SDL}
#app environment building
.PHONY: mapDrawer

mapDrawer:
	${CMD_COMPILATION} -o ${DIR_EXECUTABLE}/mapDrawer -I${DIR_PROJECT} \
	${DIR_MAIN}/mapDrawer.cpp \
	${DIR_PROJECT}/src/system/_base/object/process/handler/image.cpp \
	${DIR_PROJECT}/src/system/_extension/nlohmann.cpp \
	${DIR_PROJECT}/src/server/process/newBiome_v1.cpp \
	${DIR_PROJECT}/src/system/_base/process/scalar.cpp \
