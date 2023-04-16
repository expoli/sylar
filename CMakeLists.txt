cmake_minimum_required(VERSION 3.26)
project(sylar)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

set(LIB_SRC
        sylar/log.cpp
        sylar/util.cpp
        sylar/config.cpp
        )

find_package(yaml-cpp REQUIRED )
# ${YAML_CPP_INCLUDE_DIR}
# ${YAML_CPP_LIBRARIES}

add_library(sylar SHARED ${LIB_SRC} ${YAML_CPP_LIBRARIES})

add_executable(test tests/test.cpp)
add_dependencies(test sylar)
target_link_libraries(test sylar)

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config sylar)
target_link_libraries(test_config sylar ${YAML_CPP_LIBRARIES})

set(CMAKE_CXX_STANDARD 11)

set(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/lib)
