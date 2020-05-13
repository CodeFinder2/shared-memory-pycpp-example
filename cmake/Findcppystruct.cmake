# Copyright (C) 2020 Adrian Böckenkamp
# This code is licensed under the BSD 3-Clause license (see LICENSE for details).
#
# Find the cppystruct include directory
# The following variables are set if cppystruct is found.
#  cppystruct_FOUND        - 1 (true) when the cppystruct include directory is found.
#  cppystruct_INCLUDE_DIR  - The path to where the cppystruct include files are.
# If cppystruct is not found, cppystruct_FOUND is set to 0 (false).

find_package(PkgConfig)

# Allow the user can specify the include directory manually:
if(NOT EXISTS "${cppystruct_INCLUDE_DIR}")
     find_path(cppystruct_INCLUDE_DIR
         NAMES cppystruct.h
         DOC "cppystruct library header files"
     )
endif()

if(EXISTS "${cppystruct_INCLUDE_DIR}")
  include(FindPackageHandleStandardArgs)
  mark_as_advanced(cppystruct_INCLUDE_DIR)
else()
  include(ExternalProject)
  ExternalProject_Add(cppystruct
    GIT_REPOSITORY https://github.com/karkason/cppystruct.git
    CMAKE_ARGS -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
    INSTALL_COMMAND "" # Disable install step, this is a header only lib!
    BUILD_COMMAND "" # dito
  )

  # Specify include dir
  ExternalProject_Get_Property(cppystruct source_dir)
  set(cppystruct_INCLUDE_DIR ${source_dir}/include)
endif()

if(EXISTS "${cppystruct_INCLUDE_DIR}")
  set(cppystruct_FOUND 1)
else()
  set(cppystruct_FOUND 0)
endif()
