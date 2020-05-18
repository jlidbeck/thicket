cmake_minimum_required(VERSION 3.17)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(Thicket)

include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

set(tree_SOURCE_DIR ${PROJECT_SOURCE_DIR}/tree)
list(APPEND CMAKE_PREFIX_PATH ${tree_SOURCE_DIR}/cmake)
ExternalProject_Add(tree
  SOURCE_DIR ${tree_SOURCE_DIR}
  )

message(CMAKE_MODULE_PATH\ ${CMAKE_PREFIX_PATH})

#set(ThicketApp_SOURCE_DIR ${PROJECT_SOURCE_DIR}/ThicketApp)
#list(APPEND CMAKE_PREFIX_PATH ${ThicketApp_SOURCE_DIR}/cmake)
#ExternalProject_Add(ThicketApp
#  SOURCE_DIR ${ThicketApp_SOURCE_DIR}
#  )

add_subdirectory(ThicketApp)

#add_subdirectory(FFillDemo)