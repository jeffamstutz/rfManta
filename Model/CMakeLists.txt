
INCLUDE (AmbientLights/CMakeLists.txt)
INCLUDE (Backgrounds/CMakeLists.txt)
INCLUDE (Cameras/CMakeLists.txt)
INCLUDE (Groups/CMakeLists.txt)
INCLUDE (Lights/CMakeLists.txt)
INCLUDE (Materials/CMakeLists.txt)
INCLUDE (Primitives/CMakeLists.txt)
INCLUDE (TexCoordMappers/CMakeLists.txt)
INCLUDE (Instances/CMakeLists.txt)
INCLUDE (MiscObjects/CMakeLists.txt)
INCLUDE (Readers/CMakeLists.txt)
INCLUDE (Intersections/CMakeLists.txt)
INCLUDE (Textures/CMakeLists.txt)

IF (FOUND_TEEM) 
  INCLUDE_DIRECTORIES(${Teem_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Teem_LIBRARY_DIRS})
ENDIF (FOUND_TEEM)

ADD_LIBRARY (Manta_Model
             ${Manta_AmbientLights_SRCS}
             ${Manta_Backgrounds_SRCS}
             ${Manta_Cameras_SRCS}
             ${Manta_Groups_SRCS}
             ${Manta_Lights_SRCS}
             ${Manta_Materials_SRCS}
             ${Manta_Primitives_SRCS}
             ${Manta_Primitives_Volume_SRCS}
             ${Manta_TexCoordMappers_SRCS}
             ${Manta_Instances_SRCS}
             ${Manta_MiscObjects_SRCS}
             ${Manta_Readers_SRCS}
             ${Manta_Intersections_SRCS}
             ${Manta_Textures_SRCS}
             )

TARGET_LINK_LIBRARIES(Manta_Model
  Manta_Image
  Manta_Interface
  Manta_Core
  ${Manta_Model_extra_libs}
  )

