CC = gcc
AR = ar
CFLAGS ?= -O2
CPPFLAGS += -Iinclude -Isrc/internal
WARNINGS = -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow
LIB = libnumerix.a
OBJECTS = build/numerix_core.o build/numerix_platform.o

ifeq ($(OS),Windows_NT)
CLI = numerix_cli.exe
TEST_EXE = numerix_tests.exe
else
CLI = numerix_cli
TEST_EXE = numerix_tests
endif

.PHONY: all test clean
all: $(LIB) $(CLI)

build:
	$(MKDIR_COMMAND)

build/numerix_core.o: src/core/numerix_core.c src/internal/numerix_internal.h include/numerix.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) -c $< -o $@

build/numerix_platform.o: src/core/numerix_platform.c src/internal/numerix_internal.h include/numerix.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) -c $< -o $@

$(LIB): $(OBJECTS)
	$(AR) rcs $@ $^

$(CLI): cli/main.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) -o $@

$(TEST_EXE): tests/test_numerix.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $< $(LIB) -o $@

test: $(TEST_EXE)
	./$(TEST_EXE) unit
	./$(TEST_EXE) integration

clean:
	$(CLEAN_COMMAND)

ifeq ($(OS),Windows_NT)
CLEAN_COMMAND = cmd /c "if exist build rmdir /s /q build & if exist libnumerix.a del /q libnumerix.a & if exist numerix_cli.exe del /q numerix_cli.exe & if exist numerix_tests.exe del /q numerix_tests.exe"
MKDIR_COMMAND = cmd /c "if not exist build mkdir build"
else
CLEAN_COMMAND = rm -rf build $(LIB) numerix_cli numerix_tests
MKDIR_COMMAND = mkdir -p build
endif
