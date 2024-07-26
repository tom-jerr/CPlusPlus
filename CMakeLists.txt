cmake_minimum_required(VERSION 3.20.0)

project(C++20Practice VERSION 0.0.1 LANGUAGES C CXX)

# ------ Standard ------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------ Build tool ------
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_GENERATOR "Unix Makefiles")

# ------ Build type ------
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo") # default build type
endif()
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(__DEBUG__) # set __DEBUG__ for debug release
endif()
if(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  add_compile_definitions(__DEBUG__) # set __DEBUG__ for debug release
endif()
if(CMAKE_BUILD_TYPE STREQUAL "RelWithSizeOp")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Oz -pipe -fno-math-errno -flto=auto")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Oz -pipe -fno-math-errno -flto=auto")
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -fno-math-errno -flto=auto")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -fno-math-errno -flto=auto")
endif()

# Export compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb -std=c++20")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -DNODEBUG -O3 -Wall -std=c++20")

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(./src)

# add_subdirectory(./test)

# add_subdirectory(./example)


