CXX=g++ --std=c++20 -Wall -Wextra
CXXFLAGS=-g  -DQT_NOT_WANTED
LIBS=-lm -lz -lminizip
BPATH=build
EXT=.o
TARGET=$(BPATH)/mcxz
DEPS=$(BPATH)/main$(EXT) $(BPATH)/tileset$(EXT) $(BPATH)/FileWrap$(EXT) $(BPATH)/Frame$(EXT) $(BPATH)/FrameSet$(EXT) $(BPATH)/PngMagic$(EXT) $(BPATH)/DotArray$(EXT) $(BPATH)/helper$(EXT) 

all: $(TARGET)

$(BPATH)/main$(EXT): src/main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@



$(BPATH)/tileset$(EXT): src/tileset.cpp src/tileset.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BPATH)/FileWrap$(EXT): ../../shared/FileWrap.cpp ../../shared/FileWrap.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BPATH)/Frame$(EXT): ../../shared/Frame.cpp ../../shared/Frame.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BPATH)/FrameSet$(EXT): ../../shared/FrameSet.cpp ../../shared/FrameSet.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BPATH)/PngMagic$(EXT): ../../shared/PngMagic.cpp ../../shared/PngMagic.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BPATH)/DotArray$(EXT): ../../shared/DotArray.cpp ../../shared/DotArray.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BPATH)/helper$(EXT): ../../shared/helper.cpp ../../shared/helper.h
	$(CXX) $(CXXFLAGS) -c $< -o $@



$(TARGET): $(DEPS)
	$(CXX) $(CXXFLAGS) $(DEPS) $(LIBS) -o $@

clean:
	rm -f $(BPATH)/*