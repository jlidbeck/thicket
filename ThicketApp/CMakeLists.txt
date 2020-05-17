cmake_minimum_required (VERSION 3.16 FATAL_ERROR)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR/cmake}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(ThicketApp LANGUAGES CXX)

#if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
#  set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR} CACHE PATH "Set install prefix to project source folder." FORCE)
#endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

message(CMAKE_MODULE_PATH\ ${CMAKE_PREFIX_PATH})
# Tests
#enable_testing()
#add_subdirectory(${PROJECT_SOURCE_DIR}/test)

#find_package(Boost)

#add_subdirectory(${PROJECT_SOURCE_DIR}/src)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenCV REQUIRED)
find_package(TreeLib REQUIRED)

set(SOURCE
  ThicketApp.cpp
  ThicketDlg.cpp
  MatView.cpp
  pch.cpp
  )

add_executable(${PROJECT_NAME} WIN32 ${SOURCE})

#set(CMAKE_CXX_EXTENSIONS OFF)
add_definitions(-D_AFXDLL)
add_definitions(-D_UNICODE)
add_definitions(-DUNICODE)
add_definitions(-DNDEBUG)
add_definitions(-D_WINDOWS)
set(CMAKE_MFC_FLAG 2)

#set_target_properties(${PROJECT_NAME} PROPERTIES
#  MSVC_RUNTIME_LIBRARY MultiThreaded
#  )

target_include_directories(${PROJECT_NAME}
  PRIVATE
    #../tree/include #TODO This needs to use the TreeLib target.
  )

target_compile_options(${PROJECT_NAME}
  PRIVATE
  )

target_link_options(${PROJECT_NAME}
  PRIVATE
  )

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    nlohmann_json
    nlohmann_json::nlohmann_json
    opencv_core
    opencv_highgui
    opencv_imgproc
    TreeLib
  )

install(
  TARGETS
    ${PROJECT_NAME}
  DESTINATION
    bin/$<CONFIG>
  )
