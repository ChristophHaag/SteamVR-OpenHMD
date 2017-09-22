cmake_minimum_required(VERSION 2.8)

project (driver_openhmd)

# meta data that the steamvr plugin needs in the base directory of the steamvr plugin
file(COPY driver.vrdrivermanifest DESTINATION ${CMAKE_BINARY_DIR})
file(COPY resources DESTINATION ${CMAKE_BINARY_DIR})

include_directories("${CMAKE_SOURCE_DIR}/external/openhmd/include")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/external/openhmd/cmake/")
add_subdirectory(./external/openhmd)

#we only need one single header file from openvr, so don't bother downloading the entire thing
file(DOWNLOAD https://raw.githubusercontent.com/ValveSoftware/openvr/master/headers/openvr_driver.h "${CMAKE_SOURCE_DIR}/external/openvr/openvr_driver.h")
include_directories("${CMAKE_SOURCE_DIR}/external/openvr/")

add_library(driver_openhmd SHARED
  driverlog.cpp
  driverlog.h
  driver_openhmd.cpp
)
target_link_libraries(driver_openhmd
  ${CMAKE_DL_LIBS}
  openhmd-shared
)

#determine the output directory for the steamvr plugin
if (WIN32)
  # FIXME need to account for different architectures
  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(PLUGINDIR ${CMAKE_BINARY_DIR}/bin/win64 CACHE TYPE INTERNAL)
  else()
    set(PLUGINDIR ${CMAKE_BINARY_DIR}/bin/win32 CACHE TYPE INTERNAL)
  endif()
elseif(APPLE)
  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(PLUGINDIR ${CMAKE_BINARY_DIR}/bin/osx64 CACHE TYPE INTERNAL)
  else()
    set(PLUGINDIR ${CMAKE_BINARY_DIR}/bin/osx32 CACHE TYPE INTERNAL)
  endif()
elseif(NOT ANDROID)
  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(PLUGINDIR ${CMAKE_BINARY_DIR}/bin/linux64 CACHE TYPE INTERNAL)
  else()
    set(PLUGINDIR ${CMAKE_BINARY_DIR}/bin/linux32 CACHE TYPE INTERNAL)
  endif()
endif()
MESSAGE("SteamVR plugin path: ${PLUGINDIR}")
set_target_properties(driver_openhmd PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${PLUGINDIR}")

SET_TARGET_PROPERTIES(driver_openhmd PROPERTIES PREFIX "") # don't add lib prefix to driver_openhmd.so

add_dependencies(driver_openhmd openhmd-shared)
