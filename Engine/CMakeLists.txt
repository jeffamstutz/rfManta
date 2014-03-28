
INCLUDE (Control/CMakeLists.txt)
INCLUDE (Display/CMakeLists.txt)
INCLUDE (IdleModes/CMakeLists.txt)
INCLUDE (ImageTraversers/CMakeLists.txt)
INCLUDE (LoadBalancers/CMakeLists.txt)
INCLUDE (PixelSamplers/CMakeLists.txt)
INCLUDE (Renderers/CMakeLists.txt)
INCLUDE (SampleGenerators/CMakeLists.txt)
INCLUDE (Shadows/CMakeLists.txt)

IF (FOUND_TEEM) 
  INCLUDE_DIRECTORIES(${Teem_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Teem_LIBRARY_DIRS})
ENDIF (FOUND_TEEM)

ADD_LIBRARY (Manta_Engine
             ${Manta_Control_SRCS}
             ${Manta_Display_SRCS}
             ${Manta_IdleModes_SRCS}
             ${Manta_ImageTraversers_SRCS}
             ${Manta_LoadBalancers_SRCS}
             ${Manta_PixelSamplers_SRCS}
             ${Manta_SampleGenerators_SRCS}
             ${Manta_Renderers_SRCS}
             ${Manta_Shadows_SRCS}
             )


TARGET_LINK_LIBRARIES(Manta_Engine Manta_Model
                                   Manta_Image
                                   Manta_Interface
                                   Manta_Core
                                   )

IF(MANTA_ENABLE_X11)
 TARGET_LINK_LIBRARIES(Manta_Engine Manta_Core_XUtils)
ENDIF(MANTA_ENABLE_X11)

TARGET_LINK_LIBRARIES(Manta_Engine ${OPENGL_LIBRARIES}
                                   ${X11_LIBRARIES}
                                   )

IF(PABST_FOUND)
  INCLUDE_DIRECTORIES(${PABST_INCLUDE_DIR})
  TARGET_LINK_LIBRARIES(Manta_Engine ${PABST_LIBRARIES})
ENDIF(PABST_FOUND)

# Build the factory library
SUBDIRS(Factory)
