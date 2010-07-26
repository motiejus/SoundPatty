# vim: set noet:
DEPS=`pkg-config --libs jack` -std=gnu++0x -g

all: main controller

main: main.o soundpatty.o input.o logger.o
	g++ ${DEPS} main.o soundpatty.o input.o logger.o -o main
main.o: main.cpp main.h input.h soundpatty.h logger.h
	g++ ${DEPS} -c main.cpp
soundpatty.o: main.h soundpatty.cpp soundpatty.h input.h logger.h
	g++ ${DEPS} -c soundpatty.cpp
input.o: main.h input.cpp input.h soundpatty.h logger.h
	g++ ${DEPS} -c input.cpp
logger.o: logger.h
	g++ ${DEPS} -c logger.cpp

clean:
	rm -f main soundpatty.o input.o main.o controller controller.o logger.o

controller: controller.o soundpatty.o input.o logger.o
	g++ ${DEPS} logger.o controller.o soundpatty.o input.o -o controller
controller.o: controller.cpp controller.h logger.o
	g++ ${DEPS} -c controller.cpp
