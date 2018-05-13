cmake_minimum_required(VERSION 3.1)

project (driver_openhmd)

set (CMAKE_CXX_STANDARD 11)

# meta data that the steamvr plugin needs in the base directory of the steamvr plugin
file(COPY driver.vrdrivermanifest DESTINATION ${CMAKE_BINARY_DIR})
file(COPY resources DESTINATION ${CMAKE_BINARY_DIR})

include_directories("${CMAKE_SOURCE_DIR}/subprojects/openhmd/include")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/subprojects/openhmd/cmake/")
add_subdirectory(./subprojects/openhmd)

include_directories("${CMAKE_SOURCE_DIR}/subprojects/openvr/")

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
