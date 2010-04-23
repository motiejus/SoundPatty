readit: readit.cpp
		g++ -g -o readit `pkg-config --cflags --libs jack` readit.cpp
clean:
		rm readit
