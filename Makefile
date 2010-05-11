all: readit soundpatty

soundpatty: soundpatty.cpp
		g++ -o soundpatty -g `pkg-config --cflags --libs jack` -lpthread soundpatty.cpp
readit: readit.cpp
		g++ -o readit -g `pkg-config --cflags --libs jack` -lpthread readit.cpp
clean:
		rm readit soundpatty
