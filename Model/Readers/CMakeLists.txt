
SET (Manta_Readers_SRCS
  Readers/glm/glm.h
  Readers/glm/glm.cc
  Readers/rply/rply.c
  Readers/IW.cc
  Readers/MReader.h
  Readers/MReader.cc
  Readers/PlyReader.cc
  Readers/V3C1.h
  Readers/V3C1.cc
  Readers/UDAReader.h
  )

IF (LIBXML2_FOUND AND ZLIB_FOUND)
     SET (Manta_Readers_SRCS ${Manta_Readers_SRCS}
          Readers/UDAReader.cc
         )
     INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE})
     INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
SET(Manta_Model_extra_libs ${Manta_Model_extra_libs}
    ${ZLIB_LIBRARY}         
    )

ELSE (LIBXML2_FOUND AND ZLIB_FOUND)
   SET (Manta_Readers_SRCS ${Manta_Readers_SRCS}
          Readers/UDAReader-stub.cc
         )

ENDIF (LIBXML2_FOUND AND ZLIB_FOUND)


# Apple places malloc.h in /usr/include/malloc/malloc.h
IF (APPLE)
  INCLUDE_DIRECTORIES(/usr/include/malloc/)
ENDIF (APPLE)

# Reader for NRRD particle data
IF(BUILD_NRRDPARTICLES)
  SET(Manta_Readers_SRCS
    ${Manta_Readers_SRCS}
    Readers/ParticleNRRD.h
    Readers/ParticleNRRD.cc
    )
  INCLUDE_DIRECTORIES(${Teem_INCLUDE_DIRS})

  SET(Manta_Model_extra_libs ${Manta_Model_extra_libs}
    ${Teem_LIBRARIES}                                   
    )
ENDIF(BUILD_NRRDPARTICLES)
