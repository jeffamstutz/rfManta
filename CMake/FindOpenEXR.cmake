# The following are set after configuration is done:
#
#  OpenEXR_FOUND          - True or false depending on if we have all the right
#                           pieces around.
#  OpenEXR_LIB            - Name of library.  Set to OpenEXR_LIB-NOTFOUND if 
#                           not found.
#  OpenEXR_H              - Location of the main include file.  This is an 
#                           extra test to make sure that we actually have the 
#                           development packages around.

SET( OpenEXR_PREFIX "" CACHE PATH "Prefix path to where OpenEXR is installed")
SET( system_path ${OpenEXR_PREFIX} "/usr/local" "/usr" )
IF ( APPLE )
  SET( system_path ${system_path} "/opt/local" )
ENDIF ( APPLE )

# Append the appropiate lib and include
SET( library_path "" )
SET( include_path "" )
FOREACH( path ${system_path} )
  SET( library_path ${library_path} "${path}/lib" )
  SET( include_path ${include_path} "${path}/include" )
ENDFOREACH( path )

FIND_LIBRARY( OpenEXR_LIB "IlmImf" ${library_path} )
FIND_LIBRARY( OpenEXR_Half_LIB "Half" ${library_path} )
FIND_LIBRARY( OpenEXR_Math_LIB "Imath" ${library_path} )
FIND_LIBRARY( OpenEXR_Iex_LIB "Iex" ${library_path} )

FIND_FILE( OpenEXR_H "OpenEXR/ImfIO.h" ${include_path} )

MARK_AS_ADVANCED( OpenEXR_LIB OpenEXR_Half_LIB OpenEXR_Math_LIB OpenEXR_Iex_LIB OpenEXR_H )

IF( OpenEXR_LIB AND OpenEXR_Half_LIB AND OpenEXR_Math_LIB AND OpenEXR_Iex_LIB AND OpenEXR_H)
  SET(OpenEXR_FOUND TRUE)
  GET_FILENAME_COMPONENT(OpenEXR_Include_Dir ${OpenEXR_H}
                         PATH)
  GET_FILENAME_COMPONENT(OpenEXR_Lib_Dir ${OpenEXR_LIB}
                         PATH)
  SET(OpenEXR_LIBRARIES ${OpenEXR_LIB} ${OpenEXR_Half_LIB} ${OpenEXR_Math_LIB} ${OpenEXR_Iex_LIB})
ELSE( OpenEXR_LIB AND OpenEXR_Half_LIB AND OpenEXR_Math_LIB AND OpenEXR_Iex_LIB AND OpenEXR_H)
  SET(OpenEXR_FOUND FALSE)
ENDIF( OpenEXR_LIB AND OpenEXR_Half_LIB AND OpenEXR_Math_LIB AND OpenEXR_Iex_LIB AND OpenEXR_H)

