cmake_minimum_required(VERSION 3.15)

project(signalTests LANGUAGES CXX)

find_package(Catch2 3 REQUIRED)
enable_testing()

add_executable(signalTests
  SignalTest.cpp
)

target_link_libraries(signalTests PRIVATE signal Catch2::Catch2WithMain)

target_compile_features(signalTests PRIVATE cxx_std_20)

include(CTest)
include(Catch)
catch_discover_tests(signalTests)
