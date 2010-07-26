# vim: set noet:
DEPS=`pkg-config --libs jack` -std=gnu++0x -g

all: main controller

main: wavinput.o logger.o main.h soundpatty.o wavinput.o jackinput.o
	g++ ${DEPS} main.cpp logger.o wavinput.o jackinput.o soundpatty.o -o main
controller: controller.o soundpatty.o wavinput.o jackinput.o logger.o
	g++ ${DEPS} logger.o controller.cpp soundpatty.o wavinput.o jackinput.o -o controller
soundpatty.o: main.h soundpatty.cpp soundpatty.h input.h logger.h
	g++ ${DEPS} -c soundpatty.cpp
wavinput.o: wavinput.cpp wavinput.h input.h
	g++ ${DEPS} -c wavinput.cpp
jackinput.o: jackinput.cpp jackinput.h input.h
	g++ ${DEPS} -c wavinput.cpp jackinput.cpp
logger.o: logger.h
	g++ ${DEPS} -c logger.cpp
main.h: types.h logger.h
input.h: main.h


clean:
	rm -f main soundpatty.o wavinput.o jackinput.o controller logger.o
