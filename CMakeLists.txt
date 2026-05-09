cmake_minimum_required(VERSION 3.15)
project(MyTemplate)

enable_testing()

# Define UNICODE
add_definitions(-DUNICODE -D_UNICODE)

add_subdirectory(src)
add_subdirectory(tests)

install(TARGETS MyTemplate
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)