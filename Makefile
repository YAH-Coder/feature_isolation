
# Variables
CXX = g++
CXXSTANDARD = -std=c++20
CXXFLAGS = -Wall -Iinclude
OUTPUT = testing
DOXYGEN = doxygen

# Targets
all: test benchmark

test: src/test.cpp include/fi_mathhelper.hpp
	$(CXX) $(CXXSTANDARD) $(CXXFLAGS) $< -o $(OUTPUT)/$@ 

benchmark: src/benchmark.cpp include/fi_mathhelper.hpp
	$(CXX) $(CXXSTANDARD) $(CXXFLAGS) $< -o $(OUTPUT)/$@ 

docs: Doxyfile
	$(DOXYGEN) $<

clean:
	rm -f $(OUTPUT)/test $(OUTPUT)/benchmark
	rm -rf doc/html

.PHONY: all docs clean


