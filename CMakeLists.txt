cmake_minimum_required(VERSION 3.1.0)

project(shared_memory_cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
  set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()
include_directories(src)

# Extend CMake's module search path to find our custom module for "cppystruct":
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
find_package(cppystruct REQUIRED)

find_package(Qt5 COMPONENTS Widgets Concurrent REQUIRED)

add_executable(${CMAKE_PROJECT_NAME}
  src/ui/dialog.ui
  src/ui/dialog.h
  src/ui/dialog.cpp
  src/ui/main.cpp
  src/prodcon_ipc/abstract_ipc.h
  src/prodcon_ipc/abstract_ipc.cpp
  src/prodcon_ipc/consumer_ipc.h
  src/prodcon_ipc/consumer_ipc.cpp
  src/prodcon_ipc/producer_ipc.h
  src/prodcon_ipc/producer_ipc.cpp
)

include_directories(SYSTEM ${cppystruct_INCLUDE_DIR})

target_link_libraries(${CMAKE_PROJECT_NAME} Qt5::Widgets Qt5::Concurrent)
add_dependencies(${CMAKE_PROJECT_NAME} cppystruct)
