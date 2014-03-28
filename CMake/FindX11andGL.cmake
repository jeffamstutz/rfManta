###############################################################################
#
# This script searches for X11 and OpenGL mostly using the standard CMake
# find scripts. It sets the following variables:
#
# OPENGL_INCLUDE     -- Location of GL/ subdirectory.
# OPENGL_TARGET_LINK -- Variable for TARGET_LINK_LIBRARY command.
#
###############################################################################


###############################################################################
# X11  X11  X11  X11  X11  X11  X11  X11  X11  X11  X11  X11  X11  X11  X11  X1

SET(MANTA_ENABLE_X11 ON CACHE BOOL "Is X11 code enabled or disabled")

IF(MANTA_ENABLE_X11)
  INCLUDE (${CMAKE_ROOT}/Modules/FindX11.cmake)
  IF (NOT X11_FOUND)
    SET(MANTA_ENABLE_X11 OFF CACHE BOOL "Is X11 code enabled or disabled" FORCE)
  ENDIF(NOT X11_FOUND)
ENDIF(MANTA_ENABLE_X11)

IF (APPLE)
  IF(MANTA_ENABLE_X11)
    # By setting these before we call FindOpenGL, we make sure we get the X11 versions
    SET (OPENGL_INCLUDE_DIR               /usr/X11R6/include)
    SET (OPENGL_gl_LIBRARY                /usr/X11R6/lib/libGL.dylib)
    SET (OPENGL_glu_LIBRARY               /usr/X11R6/lib/libGLU.dylib)
  ENDIF (MANTA_ENABLE_X11)
ENDIF (APPLE)

IF (CYGWIN)
  IF(MANTA_ENABLE_X11)
    SET (OPENGL_INCLUDE_DIR               /usr/X11R6/include)
    IF (BUILD_SHARED_LIBS)
      SET (OPENGL_gl_LIBRARY              /usr/X11R6/bin/libGL.dll)
      SET (OPENGL_glu_LIBRARY             /usr/X11R6/bin/libGLU.dll)
    ELSE (BUILD_SHARED_LIBS)
      SET (OPENGL_gl_LIBRARY              /usr/X11R6/lib/libGL.dll.a)
      SET (OPENGL_glu_LIBRARY             /usr/X11R6/lib/libGLU.dll.a)
    ENDIF (BUILD_SHARED_LIBS)
  ENDIF(MANTA_ENABLE_X11)
ENDIF (CYGWIN)

INCLUDE (${CMAKE_ROOT}/Modules/FindOpenGL.cmake)

SET(OPENGL_INCLUDE        ${OPENGL_INCLUDE_DIR})
SET(OPENGL_LINK_LIBRARIES ${OPENGL_LIBRARIES})

