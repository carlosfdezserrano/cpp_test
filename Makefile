CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

.PHONY: all part1 part2 clean

all: part1.out part2.out

part1.out: part1/part1.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

part2.out: part2/part2.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ -lssl -lcrypto -pthread

clean:
	rm -f *.out