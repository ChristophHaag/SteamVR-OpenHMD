cmake_minimum_required(VERSION 2.8)

project (driver_openhmd)

file(COPY driver.vrdrivermanifest DESTINATION ${CMAKE_BINARY_DIR})
file(COPY resources DESTINATION ${CMAKE_BINARY_DIR})

set(CMAKE_C_FLAGS "-std=c99")
#set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(./external/openvr)
add_subdirectory(./external/openhmd)

include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${OPENVR_INCLUDE_DIRS})
include_directories(${OPENHMD_INCLUDE_DIRS})

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

MESSAGE("PLUGIN DIR: ${PLUGINDIR}")

add_library(driver_openhmd SHARED
  driverlog.cpp
  driverlog.h
  driver_openhmd.cpp
)
target_link_libraries(driver_openhmd
  ${CMAKE_DL_LIBS}
  ${OPENHMD_LINK_DIR}/libopenhmd.a
)
# don't add lib prefix to driver_openhmd.so
SET_TARGET_PROPERTIES(driver_openhmd PROPERTIES PREFIX "")
set_target_properties( driver_openhmd PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${PLUGINDIR}")
add_dependencies(driver_openhmd openvr)
add_dependencies(driver_openhmd openhmd)
