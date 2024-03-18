
# Variables
CXX = g++
CXXSTANDARD = -std=c++20
CXXFLAGS = -Wall -Iinclude
OUTPUT_DIR = testing
OUTPUT_DIR_MATH = mathhelper
DOXYGEN = doxygen

# Targets
all: test benchmark

test: src/mathhelper_test.cpp include/fi_mathhelper.hpp
	mkdir -p $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/
	$(CXX) $(CXXSTANDARD) $(CXXFLAGS) $< -o $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/$@ 
	./$(OUTPUT_DIR)/$@

benchmark: src/mathhelper_benchmark.cpp include/fi_mathhelper.hpp
	mkdir -p $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/
	$(CXX) $(CXXSTANDARD) $(CXXFLAGS) $< -o $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/$@ 

docs: Doxyfile
	$(DOXYGEN) $<

clean:
	rm -f $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/test $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/benchmark
	rm -rf docs/html

.PHONY: all docs clean


