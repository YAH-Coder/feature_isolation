
# Variables
CXX = g++
CXXSTANDARD = -std=c++20
CXXFLAGS = -Wall -Iinclude
OUTPUT_DIR = testing
DOXYGEN = doxygen

# Targets
all: test benchmark

test: src/test.cpp include/fi_mathhelper.hpp
	mkdir -p $(OUTPUT_DIR)
	$(CXX) $(CXXSTANDARD) $(CXXFLAGS) $< -o $(OUTPUT_DIR)/$@ 
	./$(OUTPUT_DIR)/$@

benchmark: src/benchmark.cpp include/fi_mathhelper.hpp
	mkdir -p $(OUTPUT_DIR)
	$(CXX) $(CXXSTANDARD) $(CXXFLAGS) $< -o $(OUTPUT_DIR)/$@ 

docs: Doxyfile
	$(DOXYGEN) $<

clean:
	rm -f $(OUTPUT_DIR)/test $(OUTPUT_DIR)/benchmark
	rm -rf docs/html

.PHONY: all docs clean


