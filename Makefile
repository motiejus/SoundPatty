readit: readit.cpp
		g++ -o readit `pkg-config --cflags --libs jack` readit.cpp
clean:
		rm readit
