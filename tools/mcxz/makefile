CXX=g++
CXXFLAGS=-g  -DQT_NOT_WANTED
LIBS=-lm -lz 
main: main.o
		$(CXX) $(CXXFLAGS) $(LIBS) src/main.cpp  ../../tilesdata.cpp  ../../map.cpp src/tileset.cpp ../../shared/FileWrap.cpp ../../shared/Frame.cpp ../../shared/FrameSet.cpp ../../shared/PngMagic.cpp ../../shared/DotArray.cpp ../../shared/helper.cpp -o mcxz 
main.o:	src/main.cpp ../../map.cpp  ../../tilesdata.cpp makefile src/tileset.cpp ../../shared/FileWrap.cpp ../../shared/Frame.cpp ../../shared/FrameSet.cpp ../../shared/PngMagic.cpp ../../shared/DotArray.cpp ../../shared/helper.cpp
