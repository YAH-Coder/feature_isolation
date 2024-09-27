# Variables
CXX = g++
CXXSTANDARD = -std=c++20
CXXFLAGS = -Wall -pedantic -Iinclude
OUTPUT_DIR = testing
OUTPUT_DIR_MATH = mathhelper
OUTPUT_DIR_SPL = spl
OUTPUT_DIR_ANALYSIS = analysis
OUTPUT_DIR_ISOLATION = isolation
DOXYGEN = doxygen

# Targets
all: test benchmark

test_math: src/mathhelper_test.cpp include/fi_mathhelper.hpp
	mkdir -p $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/
	$(CXX) $(CXXSTANDARD) $(CXXFLAGS) $< -o $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/$@ 
	./$(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/$@

test_spl: src/spl_test.cpp include/spl.hpp
	mkdir -p $(OUTPUT_DIR)/$(OUTPUT_DIR_SPL)/
	$(CXX) $(CXXSTANDARD) $(CXXFLAGS) $< -o $(OUTPUT_DIR)/$(OUTPUT_DIR_SPL)/$@ -lz3 -lfmt
	./$(OUTPUT_DIR)/$(OUTPUT_DIR_SPL)/$@

test_analysis: src/analysis_test.cpp src/feature_analysis.cpp
	mkdir -p $(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/
	$(CXX) $(CXXSTANDARD) $(CXXFLAGS) -c src/feature_analysis.cpp -o $(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/feature_analysis.o
	$(CXX) $(CXXSTANDARD) $(CXXFLAGS) -c src/analysis_test.cpp -o $(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/analysis_test.o
	$(CXX) $(CXXSTANDARD) $(CXXFLAGS) $(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/feature_analysis.o $(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/analysis_test.o -o $(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/test_analysis
	./$(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/test_analysis

test_isolation: src/isolation_test.cpp src/feature_analysis.cpp include/isolation.hpp
	mkdir -p $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/
	$(CXX) $(CXXSTANDARD) $(CXXFLAGS) -c src/feature_analysis.cpp -o $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/feature_analysis.o
	$(CXX) $(CXXSTANDARD) $(CXXFLAGS) -c src/isolation_test.cpp -o $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/isolation_test.o
	$(CXX) $(CXXSTANDARD) $(CXXFLAGS) $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/feature_analysis.o $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/isolation_test.o -lz3 -lfmt -o $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/test_isolation
	./$(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/test_isolation

test: test_math test_spl test_analysis test_isolation

benchmark: src/mathhelper_benchmark.cpp include/fi_mathhelper.hpp
	mkdir -p $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/
	$(CXX) $(CXXSTANDARD) $(CXXFLAGS) $< -o $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/$@ 

docs: Doxyfile
	$(DOXYGEN) $<

clean:
	rm -f $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/test_math $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/benchmark
	rm -f $(OUTPUT_DIR)/$(OUTPUT_DIR_SPL)/test_spl
	rm -f $(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/test_analysis $(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/feature_analysis.o $(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/analysis_test.o
	rm -f $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/test_isolation $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/feature_analysis.o $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/isolation_test.o
	rm -rf docs/html

.PHONY: all docs clean test test_math test_spl test_analysis test_isolation benchmark
