CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -g -pthread -I../include

TARGET = bench

all: $(TARGET)

$(TARGET): bench.cc
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
	rm -rf logs

.PHONY: all clean
