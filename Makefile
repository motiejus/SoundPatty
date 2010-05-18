# vim: set noet:
DEPS=`pkg-config --libs jack` -lpthread -std=gnu++0x -g

all: main controller

controller: controller.cpp controller.h
	g++ ${DEPS} controller.cpp -o controller

main: main.o soundpatty.o input.o
	g++ ${DEPS} main.o soundpatty.o input.o -o main
main.o: main.cpp main.h soundpatty.h
	g++ ${DEPS} -c main.cpp
soundpatty.o: soundpatty.cpp soundpatty.h
	g++ ${DEPS} -c soundpatty.cpp
input.o: input.cpp main.h input.h soundpatty.h
	g++ ${DEPS} -c input.cpp
clean:
	rm -f main soundpatty.o input.o main.o controller controller.o