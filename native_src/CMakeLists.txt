cmake_minimum_required(VERSION 3.7)
project(nativeETF)

set(CMAKE_CXX_STANDARD 14)
add_definitions(--std=c++14)

find_package(PythonLibs)
include_directories(
        ${CMAKE_SOURCE_DIR}/libcxx/
        ${PYTHON_INCLUDE_DIRS})

set(SOURCE_FILES
        library.cpp
        library.h

        libcxx/Src/Python3/cxxextensions.c
        libcxx/Src/Python3/cxx_extensions.cxx
        libcxx/Src/Python3/cxx_exceptions.cxx
        libcxx/Src/Python3/cxxsupport.cxx

        libcxx/Src/IndirectPythonInterface.cxx

        # libcxx/Src/cxx_exceptions.cxx
        # libcxx/Src/cxxsupport.cxx
        byte_codec.h)

add_library(nativeETF SHARED ${SOURCE_FILES})
link_libraries(nativeETF ${PYTHON_LIBRARIES})

set_target_properties(nativeETF PROPERTIES
        PREFIX ""
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/.."
        )
