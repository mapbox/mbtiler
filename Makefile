CXX := $(CXX)
CXXFLAGS := $(CXXFLAGS)
LDFLAGS := $(LDFLAGS) -lsqlite3
COMMON_FLAGS = -Wall -Wsign-compare -Wshadow -Wsign-conversion -Wconversion -Wunused-parameter -Wconversion
RELEASE_FLAGS = $(COMMON_FLAGS) -DNDEBUG -O3 -finline-functions
DEBUG_FLAGS = $(COMMON_FLAGS) -g -DDEBUG -O0

all: mbtiler

mbtiler: mbtiler.cpp Makefile
	$(CXX) -o mbtiler mbtiler.cpp $(DEBUG_FLAGS) $(CXXFLAGS) $(LDFLAGS)

test:
	rm -rf test.mbtiles
	./mbtiler test "MBTiler test dataset"
	echo "select * from metadata;" | sqlite3 test.mbtiles;
	echo "select * from tiles;" | sqlite3 test.mbtiles

clean:
	rm -f mbtiler

.PHONY: test

