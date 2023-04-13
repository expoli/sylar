cmake_minimum_required(VERSION 3.26)
project(sylar)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

set(LIB_SRC
        sylar/log.cpp
        )

add_library(sylar SHARED ${LIB_SRC})

add_executable(test tests/test.cpp)
add_dependencies(test sylar)

target_link_libraries(test sylar)

set(CMAKE_CXX_STANDARD 11)

set(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH  ${LIBRARY_OUTPUT_PATH}/lib)
