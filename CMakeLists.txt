cmake_minimum_required(VERSION 3.26)
project(sylar)

include(cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

set(LIB_SRC
        sylar/log.cpp
        sylar/util.cpp
        sylar/config.cpp
        sylar/thread.cpp
        sylar/sylar.h)

find_package(yaml-cpp REQUIRED )
# ${YAML_CPP_INCLUDE_DIR}
# ${YAML_CPP_LIBRARIES}

set(LIB_LIB
        sylar
        pthread
        ${YAML_CPP_LIBRARIES}
        )

add_library(sylar SHARED ${LIB_SRC} )
force_redefine_file_macro_for_sources(sylar)    # __FILE__ 重定义为 __FILE__ 变为相对路径

add_executable(test tests/test.cpp)
add_dependencies(test sylar)
force_redefine_file_macro_for_sources(test) # __FILE__
target_link_libraries(test ${LIB_LIB})

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config sylar)
force_redefine_file_macro_for_sources(test_config)  # __FILE__
target_link_libraries(test_config ${LIB_LIB})

add_executable(test_thread tests/test_thread.cpp)
add_dependencies(test_thread sylar)
target_link_libraries(test_thread ${LIB_LIB})

add_executable(test_util tests/test_util.cpp)
add_dependencies(test_util sylar)
force_redefine_file_macro_for_sources(test_util)  # __FILE__
target_link_libraries(test_util ${LIB_LIB})

set(CMAKE_CXX_STANDARD 11)

set(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/lib)
