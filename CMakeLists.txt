cmake_minimum_required(VERSION 3.15)
project(signal VERSION 0.1.0 LANGUAGES CXX)

include(CMakePackageConfigHelpers)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)

install(TARGETS signal
    EXPORT signalTargets
    INCLUDES DESTINATION include
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
    PATTERN "CMakeLists.txt" EXCLUDE
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/signalConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/signalConfig.cmake
    INSTALL_DESTINATION lib/cmake/signal
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/signalConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(EXPORT signalTargets
    FILE signalTargets.cmake
    NAMESPACE sg::
    DESTINATION lib/cmake/signal
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/signalConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/signalConfigVersion.cmake
    DESTINATION lib/cmake/signal
)
