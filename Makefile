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
	src/algorithms/ode/ode.c
VPATH = $(sort $(dir $(SOURCES)))
OBJECTS = $(addprefix build/,$(notdir $(SOURCES:.c=.o)))

ifeq ($(OS),Windows_NT)
CLI = numerix_cli.exe
TEST_EXE = numerix_tests.exe
LDLIBS =
else
CLI = numerix_cli
TEST_EXE = numerix_tests
LDLIBS = -lm
endif

.PHONY: all test clean
all: $(LIB) $(CLI)

build:
	$(MKDIR_COMMAND)

build/%.o: %.c src/internal/numerix_internal.h include/numerix.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) -c $< -o $@

$(LIB): $(OBJECTS)
	$(AR) rcs $@ $^

$(CLI): cli/main.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

$(TEST_EXE): tests/test_numerix.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) $(LDLIBS) -o $@

numerix_phase1_tests: tests/test_phase1.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) -o $@

test: $(TEST_EXE) numerix_phase1_tests
	./$(TEST_EXE) unit
	./$(TEST_EXE) integration
	./numerix_phase1_tests root_finding
	./numerix_phase1_tests linear_algebra
	./numerix_phase1_tests differentiation
	./numerix_phase1_tests interpolation
	./numerix_phase1_tests integration
	./numerix_phase1_tests optimization
	./numerix_phase1_tests ode

clean:
	$(CLEAN_COMMAND)

ifeq ($(OS),Windows_NT)
CLEAN_COMMAND = cmd /c "if exist build rmdir /s /q build & if exist libnumerix.a del /q libnumerix.a & if exist numerix_cli.exe del /q numerix_cli.exe & if exist numerix_tests.exe del /q numerix_tests.exe & if exist numerix_phase1_tests.exe del /q numerix_phase1_tests.exe & if exist numerix_tests_direct.exe del /q numerix_tests_direct.exe & if exist build_core.o del /q build_core.o & if exist build_platform.o del /q build_platform.o"
MKDIR_COMMAND = cmd /c "if not exist build mkdir build"
else
CLEAN_COMMAND = rm -rf build $(LIB) numerix_cli numerix_tests numerix_phase1_tests numerix_tests_direct.exe build_core.o build_platform.o
MKDIR_COMMAND = mkdir -p build
endif
