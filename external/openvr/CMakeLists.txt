include(ExternalProject)
ExternalProject_Add(
  openvr
  URL https://github.com/ValveSoftware/openvr/archive/v1.0.10.tar.gz
  URL_MD5 6ab903a6ff347f133fb7754e5fbb64f6
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
)
ExternalProject_Get_Property(openvr SOURCE_DIR)
set(OPENVR_INCLUDE_DIRS ${SOURCE_DIR}/headers CACHE TYPE INTERNAL)
#MESSAGE("OpenVR include path: ${OPENVR_INCLUDE_DIRS}")
