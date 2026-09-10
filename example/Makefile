CXX = g++
CXXFLAGS = -std=c++17 -Wall -g -pthread -I../include

TARGETS = demo_sync demo_async

all: $(TARGETS)

demo_sync: demo_sync.cc
	$(CXX) $(CXXFLAGS) -o $@ $^

demo_async: demo_async.cc
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGETS)
	rm -rf logs

.PHONY: all clean
