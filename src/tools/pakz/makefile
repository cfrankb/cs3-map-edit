CXX=g++
CXXFLAGS=-g -std=c++20 -DQT_NOT_WANTED
LIBS=-lm -lz
BPATH=build
EXT=.o
TARGET=$(BPATH)/pakz
DEPS=$(BPATH)/FileWrap$(EXT) $(BPATH)/main$(EXT) $(BPATH)/map$(EXT) $(BPATH)/maparch$(EXT)

all: $(TARGET)

$(BPATH)/FileWrap$(EXT): ../../shared/FileWrap.cpp ../../shared/FileWrap.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BPATH)/main$(EXT): src/main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BPATH)/map$(EXT): src/map.cpp src/map.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BPATH)/maparch$(EXT): src/maparch.cpp src/maparch.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(DEPS)
	$(CXX) $(CXXFLAGS) $(DEPS) $(LIBS) -o $@

clean:
	rm -f $(BPATH)/*