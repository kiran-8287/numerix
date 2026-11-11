cmake_minimum_required(VERSION 3.16)
project(NUMERIXInstalledConsumer LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_path(NUMERIX_INCLUDE_DIRECTORY numerix.h
    PATHS "${NUMERIX_INSTALL_PREFIX}/include" NO_DEFAULT_PATH)
find_library(NUMERIX_INSTALLED_LIBRARY NAMES numerix
    PATHS "${NUMERIX_INSTALL_PREFIX}/lib" "${NUMERIX_INSTALL_PREFIX}/lib64"
    NO_DEFAULT_PATH)
if(NOT NUMERIX_INCLUDE_DIRECTORY OR NOT NUMERIX_INSTALLED_LIBRARY)
    message(FATAL_ERROR "Provide a complete NUMERIX_INSTALL_PREFIX")
endif()
add_library(numerix_installed STATIC IMPORTED)
set_target_properties(numerix_installed PROPERTIES
    IMPORTED_LOCATION "${NUMERIX_INSTALLED_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${NUMERIX_INCLUDE_DIRECTORY}")
if(NOT WIN32)
    set_property(TARGET numerix_installed PROPERTY INTERFACE_LINK_LIBRARIES m)
endif()
add_executable(consumer_c main.c)
add_executable(consumer_cpp main.cpp)
target_link_libraries(consumer_c PRIVATE numerix_installed)
target_link_libraries(consumer_cpp PRIVATE numerix_installed)
enable_testing()
add_test(NAME installed_C COMMAND consumer_c)
add_test(NAME installed_Cpp COMMAND consumer_cpp)
