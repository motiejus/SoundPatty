all: readit soundpatty

soundpatty: soundpatty.cpp
		g++ -o soundpatty `pkg-config --cflags --libs jack` -lpthread soundpatty.cpp
readit: readit.cpp
		g++ -o readit `pkg-config --cflags --libs jack` -lpthread readit.cpp
clean:
		rm readit soundpatty
