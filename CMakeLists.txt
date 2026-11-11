cmake_minimum_required(VERSION 3.16)
project(NUMERIX VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(numerix STATIC
    src/core/numerix_core.c
    src/core/numerix_platform.c
)
target_include_directories(numerix PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(numerix PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/internal)
if(MSVC)
    target_compile_options(numerix PRIVATE /W4)
else()
    target_compile_options(numerix PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

add_executable(numerix_cli cli/main.c)
target_link_libraries(numerix_cli PRIVATE numerix)

include(CTest)
if(BUILD_TESTING)
    add_executable(numerix_tests tests/test_numerix.c)
    target_link_libraries(numerix_tests PRIVATE numerix)
    if(NOT MSVC)
        target_compile_options(numerix_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()
    add_test(NAME numerix_unit COMMAND numerix_tests unit)
    add_test(NAME numerix_integration COMMAND numerix_tests integration)
endif()
