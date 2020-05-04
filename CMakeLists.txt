cmake_minimum_required(VERSION 3.1.0)

project(shared_memory_cpp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
  set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Qt5 COMPONENTS Widgets Concurrent REQUIRED)

add_executable(${CMAKE_PROJECT_NAME}
  dialog.ui
  dialog.h
  dialog.cpp
  main.cpp
)

target_link_libraries(${CMAKE_PROJECT_NAME} Qt5::Widgets Qt5::Concurrent)
