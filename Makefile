OS:=$(shell uname -s)

LDFLAGS:=$(LDFLAGS)

GDAL_CXXFLAGS=$(shell gdal-config --cflags)
GDAL_LDFLAGS=$(shell gdal-config --libs --dep-libs)
CXXFLAGS:=$(CXXFLAGS)
COMMON_FLAGS=-Wall -Wsign-compare -Wshadow -Wsign-conversion -Wconversion -Wunused-parameter -Wconversion
RELEASE_FLAGS=$(COMMON_FLAGS) -DNDEBUG -O3 -finline-functions
DEBUG_FLAGS=$(COMMON_FLAGS) -g -DDEBUG -O0

all: mbtiler

mbtiler: mbtiler.cpp Makefile
	$(CXX) -o mbtiler mbtiler.cpp $(DEBUG_FLAGS) -I/usr/local/include/json $(GDAL_CXXFLAGS) $(CXXFLAGS) $(GDAL_LDFLAGS) $(LDFLAGS)

#test:

clean:
	rm -f mbtiler

.PHONY: test

