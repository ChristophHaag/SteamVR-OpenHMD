# - Check for the presence of openvr
#
# The following variables are set when openvr is found:
#  HAVE_openvr       = Set to true, if all components of openvr have been found.                      
#  openvr_INCLUDES   = Include path for the header files of openvr
#  openvr_LIBRARIES  = Link these to use openvr

## -----------------------------------------------------------------------------
## Check for the header files

find_path (openvr_INCLUDES openvr/openvr.h
  PATHS /usr/local/include /usr/include /sw/include
  )

## -----------------------------------------------------------------------------
## Check for the library

find_library (openvr_LIBRARIES libopenvr_api.a
  PATHS /usr/local/lib /usr/lib /lib /sw/lib
  )

## -----------------------------------------------------------------------------
## Actions taken when all components have been found

if (openvr_INCLUDES AND openvr_LIBRARIES)
  set (HAVE_openvr TRUE)
else (openvr_INCLUDES AND openvr_LIBRARIES)
  if (NOT openvr_FIND_QUIETLY)
    if (NOT openvr_INCLUDES)
      message (STATUS "Unable to find openvr header files!")
    endif (NOT openvr_INCLUDES)
    if (NOT openvr_LIBRARIES)
      message (STATUS "Unable to find openvr library files!")
    endif (NOT openvr_LIBRARIES)
  endif (NOT openvr_FIND_QUIETLY)
endif (openvr_INCLUDES AND openvr_LIBRARIES)

if (HAVE_openvr)
  if (NOT openvr_FIND_QUIETLY)
    message (STATUS "Found components for openvr")
    message (STATUS "openvr_INCLUDES = ${openvr_INCLUDES}")
    message (STATUS "openvr_LIBRARIES = ${openvr_LIBRARIES}")
  endif (NOT openvr_FIND_QUIETLY)
else (HAVE_openvr)
  if (openvr_FIND_REQUIRED)
    message (FATAL_ERROR "Could not find openvr!")
  endif (openvr_FIND_REQUIRED)
endif (HAVE_openvr)

mark_as_advanced (
  HAVE_openvr
  openvr_LIBRARIES
  openvr_INCLUDES
  )