CXX = g++
CXXFLAGS = -std=c++20 -O3 -Wall -g -pthread -I../include

SRCS = test_main.cc \
       test_level.cc \
       test_util.cc \
       test_buffer.cc \
       test_formatter.cc \
       test_sink.cc \
       test_looper.cc \
       test_manager.cc \
       test_integration.cc

TARGET = test_runner

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TARGET)
	./$(TARGET)

clean:
	-rm -f $(TARGET) $(TARGET).exe
	-rm -rf test_run_dir

.PHONY: all test clean
