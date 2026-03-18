# Compiler and tools
CXX ?= g++
DOXYGEN ?= doxygen
MKDIR_P ?= mkdir -p
RM ?= rm -f
RMDIR ?= rm -rf

# Standards and flags
CXXSTANDARD := -std=c++23
CPPFLAGS += -Iinclude
CXXFLAGS += $(CXXSTANDARD) -Wall -pedantic -g -O0

SANITIZE ?= 0
ifeq ($(SANITIZE),1)
CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
LDFLAGS  += -fsanitize=address
endif

# Directories
OUTPUT_DIR := testing
GENERATED_DIR := $(OUTPUT_DIR)/generated
MATH_DIR := $(OUTPUT_DIR)/mathhelper
SPL_DIR := $(OUTPUT_DIR)/spl
ANALYSIS_DIR := $(OUTPUT_DIR)/analysis
ISOLATION_DIR := $(OUTPUT_DIR)/isolation

# Executable prefix
ifeq ($(OS),Windows_NT)
    EXE_PREFIX :=
else
    EXE_PREFIX := ./
endif

# Sources and dependencies
MATH_TEST_SRC := src/mathhelper_test.cpp
MATH_TEST_DEPS := include/fi_mathhelper.hpp
MATH_BENCH_SRC := src/mathhelper_benchmark.cpp
MATH_BENCH_DEPS := include/fi_mathhelper.hpp
THEORY_BENCH_SRC := src/theory_benchmark.cpp
THEORY_BENCH_DEPS := include/spl.hpp include/feature_analysis.hpp include/isolation.hpp

SPL_TEST_SRC := src/spl_test.cpp
SPL_SRC := src/spl.cpp
SPL_ANALYSIS_SRC := src/feature_analysis.cpp
SPL_DEPS := include/spl.hpp include/feature_analysis.hpp

ANALYSIS_TEST_SRC := src/analysis_test.cpp
ANALYSIS_SRC := src/feature_analysis.cpp

ISOLATION_TEST_SRC := src/isolation_test.cpp
ISOLATION_DEPS := include/isolation.hpp
ISOLATION_CLI_SRC := src/feature_isolation.cpp

# Final executables
TEST_MATH_BIN := $(MATH_DIR)/test_math
BENCHMARK_BIN := $(MATH_DIR)/benchmark
THEORY_BENCHMARK_BIN := $(ANALYSIS_DIR)/theory_benchmark
TEST_SPL_BIN := $(SPL_DIR)/test_spl
TEST_ANALYSIS_BIN := $(ANALYSIS_DIR)/test_analysis
TEST_ISOLATION_BIN := $(ISOLATION_DIR)/test_isolation
FEATURE_ISOLATION_CLI_BIN := $(GENERATED_DIR)/feature_isolation_cli

# Object files
SPL_OBJS := $(SPL_DIR)/spl.o $(SPL_DIR)/feature_analysis.o $(SPL_DIR)/spl_test.o
ANALYSIS_OBJS := $(ANALYSIS_DIR)/feature_analysis.o $(ANALYSIS_DIR)/analysis_test.o
ISOLATION_OBJS := $(ISOLATION_DIR)/feature_analysis.o $(ISOLATION_DIR)/spl.o $(ISOLATION_DIR)/isolation_test.o
CLI_OBJS := $(GENERATED_DIR)/spl.o $(GENERATED_DIR)/feature_analysis.o $(GENERATED_DIR)/feature_isolation.o

# Default target
.DEFAULT_GOAL := all

# User-facing targets
all: test benchmark

BENCH_CXXFLAGS := $(CXXSTANDARD) -Wall -pedantic -O2 -DNDEBUG

test: test_math test_spl test_analysis test_isolation

test_math: $(TEST_MATH_BIN)
	$(EXE_PREFIX)$(TEST_MATH_BIN)

test_spl: $(TEST_SPL_BIN)
	$(MKDIR_P) $(GENERATED_DIR)
	$(EXE_PREFIX)$(TEST_SPL_BIN)

test_analysis: $(TEST_ANALYSIS_BIN)
	$(EXE_PREFIX)$(TEST_ANALYSIS_BIN)

test_isolation: $(TEST_ISOLATION_BIN)
	$(EXE_PREFIX)$(TEST_ISOLATION_BIN)

feature_isolation_cli: $(FEATURE_ISOLATION_CLI_BIN)

benchmark: $(BENCHMARK_BIN) $(THEORY_BENCHMARK_BIN)

benchmark_theory: $(THEORY_BENCHMARK_BIN)

docs: Doxyfile
	$(DOXYGEN) $<

help:
	@echo "Usage: make [target]"
	@echo
	@echo "Targets:"
	@echo "  all            Build and run all tests, then build benchmark"
	@echo "  test           Build and run all tests"
	@echo "  test_math      Build and run the mathhelper test"
	@echo "  test_spl       Build and run the SPL test"
	@echo "  test_analysis  Build and run the analysis test"
	@echo "  test_isolation Build and run the isolation test"
	@echo "  feature_isolation_cli Build only the feature isolation CLI binary"
	@echo "  benchmark      Build the benchmark executables"
	@echo "  benchmark_theory Build the theory benchmark executable"
	@echo "  docs           Generate documentation with Doxygen (optional)"
	@echo "  SANITIZE=1     Enable AddressSanitizer for builds"
	@echo "  clean          Remove build artifacts"
	@echo "  help           Show this help message"

clean:
	$(RM) $(TEST_MATH_BIN) $(BENCHMARK_BIN) $(THEORY_BENCHMARK_BIN)
	$(RM) $(TEST_SPL_BIN)
	$(RM) $(TEST_ANALYSIS_BIN) $(ANALYSIS_OBJS)
	$(RM) $(TEST_ISOLATION_BIN) $(ISOLATION_OBJS)
	$(RM) $(FEATURE_ISOLATION_CLI_BIN)
	$(RMDIR) docs/html

# Directory rules
$(MATH_DIR) $(SPL_DIR) $(ANALYSIS_DIR) $(ISOLATION_DIR) $(GENERATED_DIR):
	$(MKDIR_P) $@

# Build rules
$(TEST_MATH_BIN): $(MATH_TEST_SRC) $(MATH_TEST_DEPS) | $(MATH_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(BENCHMARK_BIN): $(MATH_BENCH_SRC) $(MATH_BENCH_DEPS) | $(MATH_DIR)
	$(CXX) $(CPPFLAGS) $(BENCH_CXXFLAGS) $< -o $@

$(THEORY_BENCHMARK_BIN): $(THEORY_BENCH_SRC) $(THEORY_BENCH_DEPS) $(SPL_SRC) $(ANALYSIS_SRC) | $(ANALYSIS_DIR)
	$(CXX) $(CPPFLAGS) $(BENCH_CXXFLAGS) $(THEORY_BENCH_SRC) $(SPL_SRC) $(ANALYSIS_SRC) -o $@ $(LDFLAGS) $(LDLIBS)

$(TEST_SPL_BIN): $(SPL_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(TEST_ANALYSIS_BIN): $(ANALYSIS_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(TEST_ISOLATION_BIN): $(ISOLATION_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(FEATURE_ISOLATION_CLI_BIN): $(CLI_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

# Object rules
$(SPL_DIR)/spl.o: $(SPL_SRC) $(SPL_DEPS) | $(SPL_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(SPL_DIR)/feature_analysis.o: $(ANALYSIS_SRC) | $(SPL_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(SPL_DIR)/spl_test.o: $(SPL_TEST_SRC) $(SPL_DEPS) | $(SPL_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(GENERATED_DIR)/spl.o: $(SPL_SRC) $(SPL_DEPS) | $(GENERATED_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(GENERATED_DIR)/feature_analysis.o: $(ANALYSIS_SRC) | $(GENERATED_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(GENERATED_DIR)/feature_isolation.o: $(ISOLATION_CLI_SRC) $(ISOLATION_DEPS) | $(GENERATED_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(ANALYSIS_DIR)/feature_analysis.o: $(ANALYSIS_SRC) | $(ANALYSIS_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(ANALYSIS_DIR)/analysis_test.o: $(ANALYSIS_TEST_SRC) | $(ANALYSIS_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(ISOLATION_DIR)/feature_analysis.o: $(ANALYSIS_SRC) | $(ISOLATION_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(ISOLATION_DIR)/spl.o: $(SPL_SRC) $(SPL_DEPS) | $(ISOLATION_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(ISOLATION_DIR)/isolation_test.o: $(ISOLATION_TEST_SRC) $(ISOLATION_DEPS) | $(ISOLATION_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: all test test_math test_spl test_analysis test_isolation feature_isolation_cli benchmark benchmark_theory docs clean help
