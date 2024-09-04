
# Variables
CXX = g++
CXXSTANDARD = -std=c++20
CXXFLAGS = -Wall -pedantic -Iinclude
OUTPUT_DIR = testing
OUTPUT_DIR_MATH = mathhelper
OUTPUT_DIR_SPL = spl
DOXYGEN = doxygen

# Targets
all: test benchmark

test_math: src/mathhelper_test.cpp include/fi_mathhelper.hpp
	mkdir -p $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/
	$(CXX) $(CXXSTANDARD) $(CXXFLAGS) $< -o $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/$@ 
	./$(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/$@

test_spl: src/spl_test.cpp include/spl_z3.hpp
	mkdir -p $(OUTPUT_DIR)/$(OUTPUT_DIR_SPL)/
	$(CXX) $(CXXSTANDARD) $(CXXFLAGS) $< -o $(OUTPUT_DIR)/$(OUTPUT_DIR_SPL)/$@ -lz3
	./$(OUTPUT_DIR)/$(OUTPUT_DIR_SPL)/$@

test: test_math test_spl

benchmark: src/mathhelper_benchmark.cpp include/fi_mathhelper.hpp
	mkdir -p $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/
	$(CXX) $(CXXSTANDARD) $(CXXFLAGS) $< -o $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/$@ 

docs: Doxyfile
	$(DOXYGEN) $<

clean:
	rm -f $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/test_math $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/benchmark
	rm -f $(OUTPUT_DIR)/$(OUTPUT_DIR_SPL)/test_spl
	rm -rf docs/html

.PHONY: all docs clean test test_math test_spl benchmark
