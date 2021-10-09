include /home/olivier/Projets/OpenLife/conf/makefile.conf

NAMESPACE =

DIR_EXECUTABLE = ${DIR_APP}
DIR_MAIN = ${DIR_TEST}
COMPILATION_FLAG = -I${DIR_SRC}/third_party/imgui `sdl2-config --cflags` `sdl2-config --libs` -lGL
#app environment building
.PHONY: default

openLifeEditor:
	g++ -Wall -o ${DIR_EXECUTABLE}/openLifeEditor -I${DIR_PROJECT} ${COMPILATION_FLAG} \
	${DIR_SRC}/tools/openLifeEditor.cpp \
	${DIR_SRC}/third_party/imgui/backends/imgui_impl_sdl.cpp \
	${DIR_SRC}/third_party/imgui/backends/imgui_impl_sdlrenderer.cpp \
	${DIR_SRC}/third_party/imgui/imgui*.cpp
