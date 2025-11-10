CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

# ---- Targets ----
all: part1 part2

part1: part1/part1.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

part2: part2/part2.cpp
	$(CXX) $(CXXFLAGS) -DCPPHTTPLIB_OPENSSL_SUPPORT $< -o $@ -lssl -lcrypto -pthread

clean:
	rm -f part1 part2