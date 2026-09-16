CXX = g++
CXXFLAGS = -std=c++20 -Wall -g -pthread -I../include

TARGETS = quickstart demo_sync demo_async custom_builder

all: $(TARGETS)

quickstart: quickstart.cc
	$(CXX) $(CXXFLAGS) -o $@ $^

demo_sync: demo_sync.cc
	$(CXX) $(CXXFLAGS) -o $@ $^

demo_async: demo_async.cc
	$(CXX) $(CXXFLAGS) -o $@ $^

custom_builder: custom_builder.cc
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	-rm -f $(TARGETS) $(addsuffix .exe, $(TARGETS))
	-rm -rf logs

.PHONY: all clean