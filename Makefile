# Variables
CXX = g++
CXXSTANDARD = -std=c++20

# Execution prefix
ifeq ($(OS),Windows_NT)
	EXE_PREFIX =
else
	EXE_PREFIX = ./
endif

# Library prefixes (override by supplying e.g., Z3_PREFIX=/custom/path on the command line)
FMT_PREFIX ?= /usr
Z3_PREFIX ?= /usr

# Use pkg-config for Z3 on POSIX systems, otherwise fallback to manual paths
ifeq ($(OS),Windows_NT)
    Z3_CFLAGS := -I$(Z3_PREFIX)/include
    Z3_LIBS   := -L$(Z3_PREFIX)/lib -lz3
else
    Z3_CFLAGS := $(shell pkg-config --cflags z3 2>/dev/null)
    ifeq ($(Z3_CFLAGS),)
        Z3_CFLAGS := -I$(Z3_PREFIX)/include
    endif
    Z3_LIBS := $(shell pkg-config --libs z3 2>/dev/null)
    ifeq ($(Z3_LIBS),)
        Z3_LIBS := -L$(Z3_PREFIX)/lib -lz3
    endif
endif

# Compiler and linker flags
CXXFLAGS += -Iinclude -I$(FMT_PREFIX)/include $(Z3_CFLAGS) -std=c++20 -Wall -pedantic
LDFLAGS += -L$(FMT_PREFIX)/lib $(Z3_LIBS)

# Output directories
OUTPUT_DIR_TESTING = testing
OUTPUT_DIR = output
OBJ_DIR = obj
OUTPUT_DIR_MATH = mathhelper
OUTPUT_DIR_SPL = spl
OUTPUT_DIR_ANALYSIS = analysis
OUTPUT_DIR_ISOLATION = isolation
DOXYGEN = doxygen

# Targets
all: test feature_isolation

test_math: src/mathhelper_test.cpp include/fi_mathhelper.hpp
	mkdir -p $(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_MATH) $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $(OBJ_DIR)/mathhelper_test.o
	$(CXX) $(CXXFLAGS) $(OBJ_DIR)/mathhelper_test.o -o $(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_MATH)/$@
	$(EXE_PREFIX)$(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_MATH)/$@

test_spl: src/spl_test.cpp src/spl.cpp include/spl.hpp
	mkdir -p $(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_SPL) $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c src/spl.cpp -o $(OBJ_DIR)/spl.o
	$(CXX) $(CXXFLAGS) -c $< -o $(OBJ_DIR)/spl_test.o
	$(CXX) $(CXXFLAGS) $(OBJ_DIR)/spl.o $(OBJ_DIR)/spl_test.o -o $(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_SPL)/$@ $(LDFLAGS)
	$(EXE_PREFIX)$(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_SPL)/$@

test_analysis: src/analysis_test.cpp src/feature_analysis.cpp
	mkdir -p $(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_ANALYSIS) $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c src/feature_analysis.cpp -o $(OBJ_DIR)/feature_analysis.o
	$(CXX) $(CXXFLAGS) -c $< -o $(OBJ_DIR)/analysis_test.o
	$(CXX) $(CXXFLAGS) $(OBJ_DIR)/feature_analysis.o $(OBJ_DIR)/analysis_test.o -o $(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_ANALYSIS)/$@ $(LDFLAGS)
	$(EXE_PREFIX)$(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_ANALYSIS)/$@

test_isolation: src/isolation_test.cpp src/feature_analysis.cpp src/spl.cpp include/isolation.hpp
	mkdir -p $(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_ISOLATION) $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c src/feature_analysis.cpp -o $(OBJ_DIR)/feature_analysis.o
	$(CXX) $(CXXFLAGS) -c src/spl.cpp -o $(OBJ_DIR)/spl.o
	$(CXX) $(CXXFLAGS) -c src/isolation_test.cpp -o $(OBJ_DIR)/isolation_test.o
	$(CXX) $(CXXFLAGS) $(OBJ_DIR)/feature_analysis.o $(OBJ_DIR)/spl.o $(OBJ_DIR)/isolation_test.o \
		-o $(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_ISOLATION)/$@ $(LDFLAGS)
	$(EXE_PREFIX)$(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_ISOLATION)/$@

feature_isolation: 
	mkdir -p $(OUTPUT_DIR) $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c src/feature_analysis.cpp -o $(OBJ_DIR)/feature_analysis.o
	$(CXX) $(CXXFLAGS) -c src/spl.cpp -o $(OBJ_DIR)/spl.o
	$(CXX) $(CXXFLAGS) -c src/feature_isolation.cpp -o $(OBJ_DIR)/feature_isolation.o
	$(CXX) $(CXXFLAGS) $(OBJ_DIR)/feature_analysis.o $(OBJ_DIR)/spl.o $(OBJ_DIR)/feature_isolation.o \
		-o $(OUTPUT_DIR)/feature_isolation $(LDFLAGS)

test: test_math test_spl test_analysis test_isolation

benchmark: src/mathhelper_benchmark.cpp include/fi_mathhelper.hpp
	mkdir -p $(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_MATH) $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $(OBJ_DIR)/mathhelper_benchmark.o
	$(CXX) $(CXXFLAGS) $(OBJ_DIR)/mathhelper_benchmark.o -o $(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_MATH)/$@
	$(EXE_PREFIX)$(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_MATH)/$@

docs: Doxyfile
	$(DOXYGEN) $<

clean:
	rm -f $(OBJ_DIR)/*.o
	rm -f $(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_MATH)/test_math $(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_MATH)/benchmark
	rm -f $(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_SPL)/test_spl
	rm -f $(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_ANALYSIS)/test_analysis
	rm -f $(OUTPUT_DIR_TESTING)/$(OUTPUT_DIR_ISOLATION)/test_isolation
	rm -f $(OUTPUT_DIR)/feature_isolation
	rm -rf docs/html

.PHONY: all docs clean test test_math test_spl test_analysis test_isolation benchmark feature_isolation
