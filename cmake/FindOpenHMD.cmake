# - Check for the presence of OpenHMD
#
# The following variables are set when OpenHMD is found:
#  HAVE_OpenHMD       = Set to true, if all components of OpenHMD have been found.                      
#  OpenHMD_INCLUDES   = Include path for the header files of OpenHMD
#  OpenHMD_LIBRARIES  = Link these to use OpenHMD

## -----------------------------------------------------------------------------
## Check for the header files

find_path (OpenHMD_INCLUDES openhmd/openhmd.h
  PATHS /usr/local/include /usr/include /sw/include
  )

## -----------------------------------------------------------------------------
## Check for the library

find_library (OpenHMD_LIBRARIES libopenhmd.so
  PATHS /usr/local/lib /usr/lib /lib /sw/lib
  )

## -----------------------------------------------------------------------------
## Actions taken when all components have been found

if (OpenHMD_INCLUDES AND OpenHMD_LIBRARIES)
  set (HAVE_OpenHMD TRUE)
else (OpenHMD_INCLUDES AND OpenHMD_LIBRARIES)
  if (NOT OpenHMD_FIND_QUIETLY)
    if (NOT OpenHMD_INCLUDES)
      message (STATUS "Unable to find OpenHMD header files!")
    endif (NOT OpenHMD_INCLUDES)
    if (NOT OpenHMD_LIBRARIES)
      message (STATUS "Unable to find OpenHMD library files!")
    endif (NOT OpenHMD_LIBRARIES)
  endif (NOT OpenHMD_FIND_QUIETLY)
endif (OpenHMD_INCLUDES AND OpenHMD_LIBRARIES)

if (HAVE_OpenHMD)
  if (NOT OpenHMD_FIND_QUIETLY)
    message (STATUS "Found components for OpenHMD")
    message (STATUS "OpenHMD_INCLUDES = ${OpenHMD_INCLUDES}")
    message (STATUS "OpenHMD_LIBRARIES = ${OpenHMD_LIBRARIES}")
  endif (NOT OpenHMD_FIND_QUIETLY)
else (HAVE_OpenHMD)
  if (OpenHMD_FIND_REQUIRED)
    message (FATAL_ERROR "Could not find OpenHMD!")
  endif (OpenHMD_FIND_REQUIRED)
endif (HAVE_OpenHMD)

mark_as_advanced (
  HAVE_OpenHMD
  OpenHMD_LIBRARIES
  OpenHMD_INCLUDES
  )