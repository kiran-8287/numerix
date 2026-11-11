CC = gcc
AR = ar
CFLAGS ?= -O2
CPPFLAGS += -Iinclude -Isrc/internal
WARNINGS = -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow
LIB = libnumerix.a
SOURCES = src/core/numerix_core.c src/core/numerix_platform.c \
	src/algorithms/root_finding/root_finding.c \
	src/algorithms/linear_algebra/linear_algebra.c \
	src/algorithms/differentiation/differentiation.c \
	src/algorithms/interpolation/interpolation.c \
	src/algorithms/integration/integration.c \
	src/algorithms/optimization/optimization.c \
	src/algorithms/ode/ode.c \
	src/expression/lexer.c \
	src/expression/parser.c \
	src/expression/evaluator.c \
	src/expression/expression.c \
	src/piecewise/piecewise.c src/piecewise/analysis.c src/piecewise/adapters.c \
	src/intelligence/problem.c src/intelligence/recommendation.c \
	src/intelligence/root_execution.c src/intelligence/portfolio.c \
	src/intelligence/execution.c src/execution/managed_execution.c \
	src/diagnostics/diagnostics.c src/diagnostics/diagnostic_analysis.c \
	src/root_discovery/root_discovery.c
VPATH = $(sort $(dir $(SOURCES)))
OBJECTS = $(addprefix build/,$(notdir $(SOURCES:.c=.o)))

ifeq ($(OS),Windows_NT)
CLI = numerix_cli.exe
EXPRESSION_DEMO = numerix_expression_demo.exe
PIECEWISE_DEMO = numerix_piecewise_demo.exe
INTELLIGENCE_DEMO = numerix_intelligence_demo.exe
MANAGED_EXECUTION_DEMO = numerix_managed_execution_demo.exe
ROOT_DISCOVERY_DEMO = numerix_root_discovery_demo.exe
TEST_EXE = numerix_tests.exe
EXPRESSION_TEST_EXE = numerix_expression_tests.exe
AUDIT_TEST_EXE = numerix_audit_regression_tests.exe
PIECEWISE_TEST_EXE = numerix_piecewise_tests.exe
INTELLIGENCE_TEST_EXE = numerix_intelligence_tests.exe
EXECUTION_TEST_EXE = numerix_execution_tests.exe
DIAGNOSTICS_TEST_EXE = numerix_diagnostics_tests.exe
ROOT_DISCOVERY_TEST_EXE = numerix_root_discovery_tests.exe
PHASE8_INTEGRATION_TEST_EXE = numerix_phase8_integration_tests.exe
ROOT_DISCOVERY_BENCHMARK = numerix_root_discovery_benchmark.exe
LDLIBS =
else
CLI = numerix_cli
EXPRESSION_DEMO = numerix_expression_demo
PIECEWISE_DEMO = numerix_piecewise_demo
INTELLIGENCE_DEMO = numerix_intelligence_demo
MANAGED_EXECUTION_DEMO = numerix_managed_execution_demo
ROOT_DISCOVERY_DEMO = numerix_root_discovery_demo
TEST_EXE = numerix_tests
EXPRESSION_TEST_EXE = numerix_expression_tests
AUDIT_TEST_EXE = numerix_audit_regression_tests
PIECEWISE_TEST_EXE = numerix_piecewise_tests
INTELLIGENCE_TEST_EXE = numerix_intelligence_tests
EXECUTION_TEST_EXE = numerix_execution_tests
DIAGNOSTICS_TEST_EXE = numerix_diagnostics_tests
ROOT_DISCOVERY_TEST_EXE = numerix_root_discovery_tests
PHASE8_INTEGRATION_TEST_EXE = numerix_phase8_integration_tests
ROOT_DISCOVERY_BENCHMARK = numerix_root_discovery_benchmark
LDLIBS = -lm
endif

.PHONY: all test clean benchmark-diagnostics benchmark-root-discovery
all: $(LIB) $(CLI) $(EXPRESSION_DEMO) $(PIECEWISE_DEMO) $(INTELLIGENCE_DEMO) $(MANAGED_EXECUTION_DEMO) $(ROOT_DISCOVERY_DEMO)

build:
	$(MKDIR_COMMAND)

build/%.o: %.c src/internal/numerix_internal.h src/internal/numerix_expression_internal.h src/internal/numerix_execution_internal.h src/piecewise/piecewise_internal.h src/intelligence/intelligence_internal.h include/numerix.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) -c $< -o $@

$(LIB): $(OBJECTS)
	$(AR) rcs $@ $^

$(CLI): cli/main.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

$(EXPRESSION_DEMO): examples/expression_demo.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

$(PIECEWISE_DEMO): examples/piecewise_demo.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

$(INTELLIGENCE_DEMO): examples/intelligence_demo.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

$(MANAGED_EXECUTION_DEMO): examples/managed_execution_demo.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

$(ROOT_DISCOVERY_DEMO): examples/root_discovery_demo.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

$(TEST_EXE): tests/test_numerix.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

numerix_phase1_tests: tests/test_phase1.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

$(AUDIT_TEST_EXE): tests/test_audit_regressions.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

$(INTELLIGENCE_TEST_EXE): tests/test_intelligence.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

$(EXECUTION_TEST_EXE): tests/test_execution.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

$(DIAGNOSTICS_TEST_EXE): tests/test_diagnostics.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

$(ROOT_DISCOVERY_TEST_EXE): tests/test_root_discovery.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

$(PHASE8_INTEGRATION_TEST_EXE): tests/test_phase8_integration.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

numerix_diagnostics_benchmark: benchmarks/diagnostics_benchmark.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

benchmark-diagnostics: numerix_diagnostics_benchmark
	./numerix_diagnostics_benchmark

$(ROOT_DISCOVERY_BENCHMARK): benchmarks/root_discovery_benchmark.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

benchmark-root-discovery: $(ROOT_DISCOVERY_BENCHMARK)
	./$(ROOT_DISCOVERY_BENCHMARK)

$(PIECEWISE_TEST_EXE): tests/test_piecewise.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

$(EXPRESSION_TEST_EXE): tests/test_expression.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

test: $(TEST_EXE) numerix_phase1_tests $(EXPRESSION_TEST_EXE) $(AUDIT_TEST_EXE) \
	$(PIECEWISE_TEST_EXE) $(INTELLIGENCE_TEST_EXE) $(EXECUTION_TEST_EXE) \
	$(DIAGNOSTICS_TEST_EXE) $(ROOT_DISCOVERY_TEST_EXE) \
	$(PHASE8_INTEGRATION_TEST_EXE)
	./$(TEST_EXE) unit
	./$(TEST_EXE) integration
	./numerix_phase1_tests root_finding
	./numerix_phase1_tests linear_algebra
	./numerix_phase1_tests differentiation
	./numerix_phase1_tests interpolation
	./numerix_phase1_tests integration
	./numerix_phase1_tests optimization
	./numerix_phase1_tests ode
	./$(EXPRESSION_TEST_EXE)
	./$(AUDIT_TEST_EXE)
	./$(PIECEWISE_TEST_EXE)
	./$(INTELLIGENCE_TEST_EXE)
	./$(EXECUTION_TEST_EXE)
	./$(DIAGNOSTICS_TEST_EXE)
	./$(ROOT_DISCOVERY_TEST_EXE)
	./$(PHASE8_INTEGRATION_TEST_EXE)

clean:
	$(CLEAN_COMMAND)
	$(PHASE8_CLEAN_COMMAND)

ifeq ($(OS),Windows_NT)
CLEAN_COMMAND = cmd /c "if exist build rmdir /s /q build & if exist libnumerix.a del /q libnumerix.a & if exist numerix_cli.exe del /q numerix_cli.exe & if exist numerix_piecewise_demo.exe del /q numerix_piecewise_demo.exe & if exist numerix_intelligence_demo.exe del /q numerix_intelligence_demo.exe & if exist numerix_managed_execution_demo.exe del /q numerix_managed_execution_demo.exe & if exist numerix_root_discovery_demo.exe del /q numerix_root_discovery_demo.exe & if exist numerix_expression_demo.exe del /q numerix_expression_demo.exe & if exist numerix_tests.exe del /q numerix_tests.exe & if exist numerix_phase1_tests.exe del /q numerix_phase1_tests.exe & if exist numerix_expression_tests.exe del /q numerix_expression_tests.exe & if exist numerix_audit_regression_tests.exe del /q numerix_audit_regression_tests.exe & if exist numerix_piecewise_tests.exe del /q numerix_piecewise_tests.exe & if exist numerix_intelligence_tests.exe del /q numerix_intelligence_tests.exe & if exist numerix_execution_tests.exe del /q numerix_execution_tests.exe & if exist numerix_diagnostics_tests.exe del /q numerix_diagnostics_tests.exe & if exist numerix_root_discovery_tests.exe del /q numerix_root_discovery_tests.exe & if exist numerix_phase8_integration_tests.exe del /q numerix_phase8_integration_tests.exe & if exist numerix_root_discovery_benchmark.exe del /q numerix_root_discovery_benchmark.exe & if exist numerix_diagnostics_benchmark.exe del /q numerix_diagnostics_benchmark.exe & if exist numerix_tests_direct.exe del /q numerix_tests_direct.exe & if exist build_core.o del /q build_core.o & if exist build_platform.o del /q build_platform.o"
PHASE8_CLEAN_COMMAND =
MKDIR_COMMAND = cmd /c "if not exist build mkdir build"
else
CLEAN_COMMAND = rm -rf build $(LIB) numerix_cli numerix_expression_demo numerix_tests numerix_phase1_tests numerix_expression_tests numerix_audit_regression_tests numerix_piecewise_demo numerix_intelligence_demo numerix_managed_execution_demo numerix_root_discovery_demo numerix_piecewise_tests numerix_intelligence_tests numerix_execution_tests numerix_diagnostics_tests numerix_root_discovery_tests numerix_root_discovery_benchmark numerix_diagnostics_benchmark numerix_tests_direct.exe build_core.o build_platform.o
PHASE8_CLEAN_COMMAND = rm -f numerix_phase8_integration_tests
MKDIR_COMMAND = mkdir -p build
endif
