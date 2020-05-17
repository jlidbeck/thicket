cmake_minimum_required (VERSION 3.16 FATAL_ERROR)
set(CMAKE_MODULE_PATH CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR/cmake}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(tree LANGUAGES CXX)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR} CACHE PATH "INSTALL_PREFIX is bin/" FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# Tests
#enable_testing()
#add_subdirectory(${PROJECT_SOURCE_DIR}/test)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenCV CONFIG REQUIRED)

#add_subdirectory(${PROJECT_SOURCE_DIR}/src)

set(SOURCE
  main.cpp
  tree.cpp
  treedemo.cpp
  )

set(HEADERS
  tree.h
  treedemo.h
  SelfLimitingPolygonTree.h
  ColorTransform.h
  util.h
  GridTree.h
  ReptileTree.h
  )

set(TREELIB_TARGET_NAME TreeLib)
add_library(${TREELIB_TARGET_NAME} STATIC tree.cpp treedemo.cpp)
target_include_directories(${TREELIB_TARGET_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  )
target_link_libraries(${TREELIB_TARGET_NAME}
  PRIVATE
    #opencv_core
    #opencv_highgui
    nlohmann_json
    nlohmann_json::nlohmann_json
  )
install(TARGETS ${TREELIB_TARGET_NAME} EXPORT TreeLibConfig
  LIBRARY DESTINATION lib
  )
install(EXPORT TreeLibConfig DESTINATION cmake)
install(FILES ${HEADERS} DESTINATION include/)

message(PROJECT_NAME\ ${PROJECT_NAME})
add_executable(${PROJECT_NAME} ${SOURCE})
#set_target_properties(${PROJECT_NAME} PROPERTIES
#  MSVC_RUNTIME_LIBRARY MultiThreadedDLL
#  )
#target_include_directories(${PROJECT_NAME} BEFORE
#  PUBLIC
#    include
#  )
target_link_options(${PROJECT_NAME}
  PRIVATE
  )
target_link_libraries(${PROJECT_NAME}
  PRIVATE
    opencv_core
    opencv_highgui
    nlohmann_json
    nlohmann_json::nlohmann_json
  )
install(
  TARGETS
    ${PROJECT_NAME}
  DESTINATION
    bin/$<CONFIG>
  )
export(
  TARGETS
    ${TREELIB_TARGET_NAME}
  FILE
    TreeLibConfig.cmake
  )