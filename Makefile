# vim: set noet:

main: main.o soundpatty.o input.o
	g++ -Wall `pkg-config --libs jack` main.o soundpatty.o input.o -o main
main.o: main.cpp
	g++ -Wall `pkg-config --libs jack` -c main.cpp
soundpatty.o: soundpatty.cpp
	g++ -Wall `pkg-config --libs jack` -c soundpatty.cpp
input.o: input.cpp
	g++ -Wall `pkg-config --libs jack` -c input.cpp
clean:
	rm -f main soundpatty.o input.o main.o


#all: readit soundpatty

#soundpatty: soundpatty.cpp
#		g++ -o soundpatty -g `pkg-config --cflags --libs jack` -lpthread soundpatty.cpp
#readit: readit.cpp
#		g++ -o readit -g `pkg-config --cflags --libs jack` -lpthread readit.cpp
#clean:
#		rm readit soundpatty

