cmake_minimum_required(VERSION 3.16.0)
project(playground VERSION 0.1.0)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(MACOSX TRUE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS NO)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(CMAKE_BUILD_TYPE Release)
add_definitions(-w)

set(SOURCE 
  InMemoryDB.h
  InMemoryDBTest.cpp
)

set(CMAKE_CXX_FLAGS "-std=c++20 -v -pthread")

add_executable(source ${SOURCE})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
