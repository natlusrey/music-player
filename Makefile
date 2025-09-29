CC = g++
CFLAGS = -g -Wall `fltk-config --cxxflags --ldflags`
all:
	$(CC) music_player.cpp $(CFLAGS) \
		-o Music_Player
clean: 
	rm -f *.o *~ *.flc
	rm -f build/*
