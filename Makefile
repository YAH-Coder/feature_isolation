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
DEPFLAGS := -MMD -MP
BENCH_CXXFLAGS := $(CXXSTANDARD) -Wall -pedantic -O2 -DNDEBUG

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
BUILD_DIRS := $(GENERATED_DIR) $(MATH_DIR) $(SPL_DIR) $(ANALYSIS_DIR) $(ISOLATION_DIR)

# Executable prefix
ifeq ($(OS),Windows_NT)
    EXE_PREFIX :=
else
    EXE_PREFIX := ./
endif

# Sources
MATH_TEST_SRC := src/mathhelper_test.cpp
MATH_BENCH_SRC := src/mathhelper_benchmark.cpp
THEORY_BENCH_SRC := src/theory_benchmark.cpp
SPL_SRC := src/spl.cpp
ANALYSIS_SRC := src/feature_analysis.cpp
SPL_TEST_SRC := src/spl_test.cpp
ANALYSIS_TEST_SRC := src/analysis_test.cpp
ISOLATION_TEST_SRC := src/isolation_test.cpp
ISOLATION_CLI_SRC := src/feature_isolation.cpp

# Final executables
TEST_MATH_BIN := $(MATH_DIR)/test_math
MATH_BENCHMARK_BIN := $(MATH_DIR)/benchmark
THEORY_BENCHMARK_BIN := $(ANALYSIS_DIR)/theory_benchmark
TEST_SPL_BIN := $(SPL_DIR)/test_spl
TEST_ANALYSIS_BIN := $(ANALYSIS_DIR)/test_analysis
TEST_ISOLATION_BIN := $(ISOLATION_DIR)/test_isolation
CLI_BIN := $(GENERATED_DIR)/cli
CLI_TEST_SCRIPT := testing/cli_regression.py

# Object files
SPL_OBJS := $(SPL_DIR)/spl.o $(SPL_DIR)/feature_analysis.o $(SPL_DIR)/spl_test.o
ANALYSIS_OBJS := $(ANALYSIS_DIR)/feature_analysis.o $(ANALYSIS_DIR)/analysis_test.o
ISOLATION_OBJS := $(ISOLATION_DIR)/feature_analysis.o $(ISOLATION_DIR)/spl.o $(ISOLATION_DIR)/isolation_test.o
CLI_OBJS := $(GENERATED_DIR)/spl.o $(GENERATED_DIR)/feature_analysis.o $(GENERATED_DIR)/feature_isolation.o
ALL_OBJS := $(sort $(SPL_OBJS) $(ANALYSIS_OBJS) $(ISOLATION_OBJS) $(CLI_OBJS))
ALL_DEPS := $(ALL_OBJS:.o=.d)

# Convenience commands
COMPILE.cxx = $(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) -c
LINK.cxx = $(CXX) $(CPPFLAGS) $(CXXFLAGS)
LINK_BENCH.cxx = $(CXX) $(CPPFLAGS) $(BENCH_CXXFLAGS)

# Default target
.DEFAULT_GOAL := all

# User-facing targets
all: test bench

cli: $(CLI_BIN)
feature_isolation_cli: cli

test: test_math test_spl test_analysis test_isolation test_cli

test_math: $(TEST_MATH_BIN)
	$(EXE_PREFIX)$(TEST_MATH_BIN)

test_spl: $(TEST_SPL_BIN) | $(GENERATED_DIR)
	$(EXE_PREFIX)$(TEST_SPL_BIN)

test_analysis: $(TEST_ANALYSIS_BIN) | $(GENERATED_DIR)
	$(EXE_PREFIX)$(TEST_ANALYSIS_BIN)

test_isolation: $(TEST_ISOLATION_BIN) | $(GENERATED_DIR)
	$(EXE_PREFIX)$(TEST_ISOLATION_BIN)

test_cli: $(CLI_BIN) | $(GENERATED_DIR)
	python3 $(CLI_TEST_SCRIPT)

bench: $(MATH_BENCHMARK_BIN) $(THEORY_BENCHMARK_BIN)
benchmark: bench
benchmark_theory: $(THEORY_BENCHMARK_BIN)

docs: Doxyfile
	$(DOXYGEN) $<

help:
	@echo "Usage: make [target]"
	@echo
	@echo "Primary targets:"
	@echo "  cli             Build the CLI executable ($(CLI_BIN))"
	@echo "  test            Build and run the full test suite"
	@echo "  bench           Build benchmark executables"
	@echo "  clean           Remove all build artifacts and generated dependency files"
	@echo "  help            Show this help message"
	@echo
	@echo "CLI quick start:"
	@echo "  ./$(CLI_BIN) --help"
	@echo "  ./$(CLI_BIN) guided"
	@echo "  ./$(CLI_BIN) plan <spl-file>"
	@echo "  ./$(CLI_BIN) analyze <spl-file> <output-file> -min-diffs=<false|true> -feature-names=<false|true>"
	@echo
	@echo "Compatibility aliases:"
	@echo "  feature_isolation_cli -> cli"
	@echo "  benchmark            -> bench"
	@echo
	@echo "Individual targets:"
	@echo "  test_math, test_spl, test_analysis, test_isolation, test_cli"
	@echo "  benchmark_theory, docs"
	@echo
	@echo "Variables:"
	@echo "  SANITIZE=1      Enable AddressSanitizer for builds"

clean:
	$(RMDIR) $(BUILD_DIRS) docs/html

# Directory rules
$(BUILD_DIRS):
	$(MKDIR_P) $@

# Auto-generated header dependencies
-include $(ALL_DEPS)

# Build rules
$(TEST_MATH_BIN): $(MATH_TEST_SRC) | $(MATH_DIR)
	$(LINK.cxx) $< -o $@ $(LDFLAGS) $(LDLIBS)

$(MATH_BENCHMARK_BIN): $(MATH_BENCH_SRC) | $(MATH_DIR)
	$(LINK_BENCH.cxx) $< -o $@ $(LDFLAGS) $(LDLIBS)

$(THEORY_BENCHMARK_BIN): $(THEORY_BENCH_SRC) $(SPL_SRC) $(ANALYSIS_SRC) | $(ANALYSIS_DIR)
	$(LINK_BENCH.cxx) $(THEORY_BENCH_SRC) $(SPL_SRC) $(ANALYSIS_SRC) -o $@ $(LDFLAGS) $(LDLIBS)

$(TEST_SPL_BIN): $(SPL_OBJS)
	$(LINK.cxx) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(TEST_ANALYSIS_BIN): $(ANALYSIS_OBJS)
	$(LINK.cxx) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(TEST_ISOLATION_BIN): $(ISOLATION_OBJS)
	$(LINK.cxx) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(CLI_BIN): $(CLI_OBJS)
	$(LINK.cxx) $^ -o $@ $(LDFLAGS) $(LDLIBS)

# Object rules with automatic dependency generation
$(SPL_DIR)/%.o: src/%.cpp | $(SPL_DIR)
	$(COMPILE.cxx) $< -o $@

$(ANALYSIS_DIR)/%.o: src/%.cpp | $(ANALYSIS_DIR)
	$(COMPILE.cxx) $< -o $@

$(ISOLATION_DIR)/%.o: src/%.cpp | $(ISOLATION_DIR)
	$(COMPILE.cxx) $< -o $@

$(GENERATED_DIR)/%.o: src/%.cpp | $(GENERATED_DIR)
	$(COMPILE.cxx) $< -o $@

.PHONY: all cli feature_isolation_cli test test_math test_spl test_analysis test_isolation test_cli bench benchmark benchmark_theory docs clean help
