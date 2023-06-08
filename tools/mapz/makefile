CXX=g++
CXXFLAGS=-g  -DQT_NOT_WANTED
LIBS=-lm -lz 
main: main.o
		$(CXX) $(CXXFLAGS) $(LIBS) src/main.cpp  ../../map.cpp ../../level.cpp ../../tilesdata.cpp src/levelarch.cpp -o mapz 
main.o:	src/main.cpp ../../map.cpp ../../level.cpp ../../tilesdata.cpp src/levelarch.cpp
