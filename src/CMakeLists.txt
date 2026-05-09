cmake_minimum_required(VERSION 3.15)

# Set the project name
project(MyTemplate LANGUAGES CXX)

# Define UNICODE
add_definitions(-DUNICODE -D_UNICODE)

# Add the library target
add_library(MyTemplate STATIC)

set_target_properties(MyTemplate PROPERTIES
  DEBUG_POSTFIX "d"
)
target_compile_definitions(MyTemplate PRIVATE UNICODE _UNICODE)

# Specify the source files for the library
target_sources(MyTemplate
  PRIVATE
    Sample.h
    Sample.cpp
)

# target_link_libraries(MyTemplate PUBLIC MyLibrary)

# Specify the include directories
target_include_directories(MyTemplate
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Set C++ standard
target_compile_features(MyTemplate PRIVATE cxx_std_20)
