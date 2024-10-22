# Variables
CXX = g++
CXXSTANDARD = -std=c++20

# Library prefixes
FMT_PREFIX := $(shell brew --prefix fmt)
Z3_PREFIX := $(shell brew --prefix z3)

# Compiler and linker flags
CXXFLAGS += -Iinclude -I$(FMT_PREFIX)/include -I$(Z3_PREFIX)/include -std=c++20 -Wall -pedantic
LDFLAGS += -L$(FMT_PREFIX)/lib -L$(Z3_PREFIX)/lib -lfmt -lz3

# Output directories
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
	$(CXX) $(CXXFLAGS) $< -o $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/$@
	./$(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/$@

test_spl: src/spl_test.cpp src/spl.cpp include/spl.hpp
	mkdir -p $(OUTPUT_DIR)/$(OUTPUT_DIR_SPL)/
	$(CXX) $(CXXFLAGS) src/spl.cpp $< -o $(OUTPUT_DIR)/$(OUTPUT_DIR_SPL)/$@ $(LDFLAGS)
	./$(OUTPUT_DIR)/$(OUTPUT_DIR_SPL)/$@

test_analysis: src/analysis_test.cpp src/feature_analysis.cpp
	mkdir -p $(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/
	$(CXX) $(CXXFLAGS) -c src/feature_analysis.cpp -o $(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/feature_analysis.o
	$(CXX) $(CXXFLAGS) -c src/analysis_test.cpp -o $(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/analysis_test.o
	$(CXX) $(CXXFLAGS) $(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/feature_analysis.o $(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/analysis_test.o -o $(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/test_analysis $(LDFLAGS)
	./$(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/test_analysis

test_isolation: src/isolation_test.cpp src/feature_analysis.cpp src/spl.cpp include/isolation.hpp
	mkdir -p $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/
	$(CXX) $(CXXFLAGS) -c src/feature_analysis.cpp -o $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/feature_analysis.o
	$(CXX) $(CXXFLAGS) -c src/spl.cpp -o $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/spl.o
	$(CXX) $(CXXFLAGS) -c src/isolation_test.cpp -o $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/isolation_test.o
	$(CXX) $(CXXFLAGS) $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/feature_analysis.o $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/spl.o $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/isolation_test.o -o $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/test_isolation $(LDFLAGS)
	./$(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/test_isolation

test: test_math test_spl test_analysis test_isolation

benchmark: src/mathhelper_benchmark.cpp include/fi_mathhelper.hpp
	mkdir -p $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/
	$(CXX) $(CXXFLAGS) $< -o $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/$@

docs: Doxyfile
	$(DOXYGEN) $<

clean:
	rm -f $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/test_math $(OUTPUT_DIR)/$(OUTPUT_DIR_MATH)/benchmark
	rm -f $(OUTPUT_DIR)/$(OUTPUT_DIR_SPL)/test_spl
	rm -f $(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/test_analysis $(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/feature_analysis.o $(OUTPUT_DIR)/$(OUTPUT_DIR_ANALYSIS)/analysis_test.o
	rm -f $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/test_isolation $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/feature_analysis.o $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/spl.o $(OUTPUT_DIR)/$(OUTPUT_DIR_ISOLATION)/isolation_test.o
	rm -rf docs/html

.PHONY: all docs clean test test_math test_spl test_analysis test_isolation benchmark
