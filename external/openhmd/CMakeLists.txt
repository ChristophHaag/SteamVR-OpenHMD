include(ExternalProject)

set(OPENHMD_DESTDIR ${CMAKE_CURRENT_BINARY_DIR})
MESSAGE("Install to ${OPENHMD_DESTDIR}")

ExternalProject_Add(
  openhmd
#  GIT_REPOSITORY https://github.com/OpenHMD/OpenHMD.git
  GIT_REPOSITORY https://github.com/ChristophHaag/OpenHMD.git
  INSTALL_COMMAND "" 
  LOG_DOWNLOAD 1
)
ExternalProject_Get_Property(openhmd SOURCE_DIR)
ExternalProject_Get_Property(openhmd BINARY_DIR)
set(OPENHMD_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE TYPE INTERNAL)
set(OPENHMD_LINK_DIR ${BINARY_DIR} CACHE TYPE INTERNAL)
#MESSAGE("OpenHMD include path: ${OPENHMD_INCLUDE_DIRS}")
#MESSAGE("OpenHMD link path: ${OPENHMD_LINK_DIR}")
