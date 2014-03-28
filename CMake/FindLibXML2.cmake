
##################################################################
# Look for libxml2

# Look for library here before you look in Thirdparty path
SET(LIBXML2_INSTALL_PATH "" CACHE PATH "Default search path for libxml2 install")

FIND_LIBRARY( LIBXML2_LIBRARY
  NAMES xml2 libxml2
  PATHS ${LIBXML2_INSTALL_PATH}/lib
        ${THIRD_PARTY_LIBRARY_PATH}
        /usr/lib
        /usr/local/lib
        /usr/lib
  DOC "libxml2 library (This is a path.)"
  )
FIND_PATH( LIBXML2_INCLUDE
  NAMES libxml/tree.h
  PATHS ${LIBXML2_INSTALL_PATH}/include
        ${THIRD_PARTY_INCLUDE_PATH}
        /usr/include/libxml2
        /usr/include
        /usr/local/include
        /usr/include
  DOC "libxml2 Include (This is a path.)"
  )

MARK_AS_ADVANCED(LIBXML2_LIBRARY LIBXML2_INCLUDE)

IF(LIBXML2_LIBRARY AND LIBXML2_INCLUDE)
  SET(LIBXML2_FOUND TRUE)
ELSE(LIBXML2_LIBRARY AND LIBXML2_INCLUDE)
  SET(LIBXML2_FOUND FALSE)
ENDIF(LIBXML2_LIBRARY AND LIBXML2_INCLUDE)

