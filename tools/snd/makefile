CXX=g++
CXXFLAGS=-g  -DQT_NOT_WANTED
LIBS=-lm -lz
BPATH=build
EXT=.o
TARGET=$(BPATH)/sndx
DEPS=$(BPATH)/main$(EXT) $(BPATH)/FileWrap$(EXT)

all: $(TARGET)

$(BPATH)/main$(EXT): src/main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BPATH)/FileWrap$(EXT): ../../shared/FileWrap.cpp ../../shared/FileWrap.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(DEPS)
	$(CXX) $(CXXFLAGS) $(DEPS) $(LIBS) -o $@

clean:
	rm -f $(BPATH)/*