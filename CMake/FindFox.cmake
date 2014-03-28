###############################################################################
# This script searches for the Fox Toolkit and populates variables:
# -- Abe Stephens.
#
# FOX_INCLUDE     -- used by INCLUDE_DIRECTORIES(${FOX_INCLUDE})
# FOX_TARGET_LINK -- used by TARGET_LINK_LIBRARIES(${FOX_TARGET_LINK})
#
# The script search for fox under:
#
# /usr/lib
# /usr/local/lib
# ${FOX_INSTALL_PATH}/lib (and) ${FOX_INSTALL_PATH}/include
# ${THIRD_PARTY_LIBRARY_PATH} (and) ${THIRD_PARTY_INCLUDE_PATH} 
#
###############################################################################

###############################################################################
# FOX  FOX  FOX  FOX  FOX  FOX  FOX  FOX  FOX  FOX  FOX  FOX  FOX  FOX  FOX  FO

# Search for the "adie" demo program to locate fox.
FIND_PATH(FOX_INSTALL_PATH adie)
IF (FOX_INSTALL_PATH)
  STRING(REGEX REPLACE "[/\\]bin$" "" FOX_INSTALL_PATH ${FOX_INSTALL_PATH})
ELSE (FOX_INSTALL_PATH)
  SET(FOX_INSTALL_PATH "" CACHE PATH "Default search path for Fox install")
ENDIF (FOX_INSTALL_PATH)

# Search for the actual fox library.
FIND_LIBRARY( FOUND_FOX_LIB     NAMES FOX-1.5 FOX-1.6
                                PATHS ${FOX_INSTALL_PATH}/lib 
                                      ${THIRD_PARTY_LIBRARY_PATH} 
                                DOC "Fox library path" )

# Seach for the fox include directory.
FIND_PATH   ( FOUND_FOX_INCLUDE fx.h 
              PATHS ${FOX_INSTALL_PATH}/include
                    ${THIRD_PARTY_INCLUDE_PATH}
              PATH_SUFFIXES fox-1.5 fox-1.6
              DOC "Fox Include path" )   


# If both were found, include fox interface code.
IF(FOUND_FOX_LIB AND FOUND_FOX_INCLUDE)
  
  # Determine other libraries to link with
  SET(FOX_X11_LIBRARIES m png)

  # Look for tiff and jpeg
  FIND_LIBRARY( FOUND_TIFF_LIB NAMES tiff PATHS /usr/lib /usr/local/lib DOC "Only required if Fox linked w/ tiff")
  IF(FOUND_TIFF_LIB)
    SET(FOX_X11_LIBRARIES ${FOX_X11_LIBRARIES} tiff)
  ENDIF(FOUND_TIFF_LIB)

  FIND_LIBRARY( FOUND_JPEG_LIB NAMES jpeg PATHS /usr/lib /usr/local/lib DOC "Only required if Fox linked w/ jpeg")
  IF(FOUND_JPEG_LIB)
    SET(FOX_X11_LIBRARIES ${FOX_X11_LIBRARIES} jpeg)
  ENDIF(FOUND_JPEG_LIB)

  # Append Xcursor if it is available.
  FIND_LIBRARY( FOUND_XCURSOR NAMES Xcursor PATHS /usr/X11R6/lib DOC "Only required if Fox linked w/ Xcursor")    
  IF(FOUND_XCURSOR)
    SET(FOX_X11_LIBRARIES ${FOX_X11_LIBRARIES}
                          Xcursor)
  ENDIF(FOUND_XCURSOR)
 
  # Append Xrandr if it is available.
  FIND_LIBRARY( FOUND_XRANDR NAMES Xrandr PATHS /usr/X11R6/lib DOC "Only required if Fox linked w/ Xrandr")    
  IF(FOUND_XRANDR)
    SET(FOX_X11_LIBRARIES ${FOX_X11_LIBRARIES}
                          Xrandr)
  ENDIF(FOUND_XRANDR)

  
  #############################################################################
  # Set the FOX_LIBS Variable.
  SET(FOX_TARGET_LINK

    ${FOUND_FOX_LIB}
    ${CMAKE_THREAD_LIBS_INIT}
    ${OPENGL_LIBRARIES} 
    ${X11_LIBRARIES} 
    ${FOX_X11_LIBRARIES}

    # Always include pthreads.
    pthread
    )   
  
  # Set the FOX_INCLUDE Variable.
  SET(FOX_INCLUDE ${FOUND_FOX_INCLUDE})

  # Mark all as advanced.
  MARK_AS_ADVANCED(
    FOX_INSTALL_PATH
    FOUND_FOX_INCLUDE
    FOUND_FOX_LIB
    FOUND_JPEG_LIB
    FOUND_TIFF_LIB
    FOUND_XCURSOR
    FOUND_XRANDR
    FOX_STATIC
    FOX_VERSION
  )    

ELSE(FOUND_FOX_LIB AND FOUND_FOX_INCLUDE)
  MESSAGE("Fox Toolkit not found.")

ENDIF(FOUND_FOX_LIB AND FOUND_FOX_INCLUDE)

