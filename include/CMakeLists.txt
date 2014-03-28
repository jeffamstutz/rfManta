##################################################################
## Configure MantaTypes.h

IF(MANTA_SSE)
  # If we want to use sse we need to have MANTA_REAL be float
  IF(MANTA_REAL STREQUAL "double")
    MESSAGE("For SSE, MANTA_REAL must be float.  Changing...")
  ENDIF(MANTA_REAL STREQUAL "double")
  SET(MANTA_REAL float CACHE STRING "Typedef for Real" FORCE)
ELSE(MANTA_SSE)
  # Set a default
  SET(MANTA_REAL float CACHE STRING "Typedef for Real")
ENDIF(MANTA_SSE)

SET(MANTA_COLOR_COMPONENT float CACHE STRING "Typedef for ColorComponent")

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/MantaTypes.h.CMakeTemplate
  ${CMAKE_BINARY_DIR}/include/MantaTypes.h
  )

###############################################################################
## Configure RayPacketParameters.h
SET(MANTA_RAYPACKET_MAXSIZE 64 CACHE STRING "Static RayPacket size.")
SET(MANTA_SCRATCHPAD_MAXSIZE 128 CACHE STRING "Static Scratchpad size.")

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/RayPacketParameters.h.CMakeTemplate
  ${CMAKE_BINARY_DIR}/include/RayPacketParameters.h
  )

###############################################################################
## Configure FragmentParameters.h
SET(MANTA_FRAGMENT_MAXSIZE  64 CACHE STRING "Static Fragment size.")

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/FragmentParameters.h.CMakeTemplate
  ${CMAKE_BINARY_DIR}/include/FragmentParameters.h
  )

###############################################################################
## Configure MantaSSE.h

IF(MANTA_SSE)
  SET(MANTA_SSE_DEF "1" CACHE INTERNAL "Turn on SSE code")
ELSE(MANTA_SSE)
  SET(MANTA_SSE_DEF "0" CACHE INTERNAL "Turn off SSE code")
ENDIF(MANTA_SSE)

IF(MANTA_SSE_GCC)
  SET(MANTA_SSE_GCC_DEF "1" CACHE INTERNAL "Has GCC 64 bit SSE extensions")
ELSE(MANTA_SSE_GCC)
  SET(MANTA_SSE_GCC_DEF "0" CACHE INTERNAL "Doesn't have GCC 64 bit SSE extensions")
ENDIF(MANTA_SSE_GCC)

IF(MANTA_SSE_CAST)
  SET(MANTA_SSE_CAST_DEF "1" CACHE INTERNAL "Casting intrinsics found.")
ELSE(MANTA_SSE_CAST)
  SET(MANTA_SSE_CAST_DEF "0" CACHE INTERNAL "No casting intrinsics found.")
ENDIF(MANTA_SSE_CAST)

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/MantaSSE.h.CMakeTemplate
  ${CMAKE_BINARY_DIR}/include/MantaSSE.h
  )

###############################################################################
## Configure TeemConfig.h

IF(FOUND_TEEM)
  SET(FOUND_TEEM_DEF "1" CACHE INTERNAL "Turn on Teem code")
ELSE(FOUND_TEEM)
  SET(FOUND_TEEM_DEF "0" CACHE INTERNAL "Turn off Teem code")
ENDIF(FOUND_TEEM)

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/TeemConfig.h.CMakeTemplate
  ${CMAKE_BINARY_DIR}/include/TeemConfig.h
  )


###############################################################################
## Configure UseStatsCollector.h

IF(USE_STATS_COLLECTOR)
  SET(USE_STATS_COLLECTOR_DEF "1" CACHE INTERNAL "Enable stats collection")
ELSE(USE_STATS_COLLECTOR)
  SET(USE_STATS_COLLECTOR_DEF "0" CACHE INTERNAL "Disable stats collection")
ENDIF(USE_STATS_COLLECTOR)

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/UseStatsCollector.h.CMakeTemplate
  ${CMAKE_BINARY_DIR}/include/UseStatsCollector.h
  )

###############################################################################
## Configure distributed version of manta for an MPI-based cluster

SET (ENABLE_MPI FALSE CACHE BOOL "Build MPI version of manta for running on clusters.")
IF (ENABLE_MPI)
  SET(USE_MPI_DEF "1" CACHE INTERNAL "Use MPI")
ELSE (ENABLE_MPI)
  SET(USE_MPI_DEF "0" CACHE INTERNAL "Use MPI")
ENDIF (ENABLE_MPI)

CONFIGURE_FILE(
  ${CMAKE_SOURCE_DIR}/include/UseMPI.h.CMakeTemplate
  ${CMAKE_BINARY_DIR}/include/UseMPI.h
)


###############################################################################
## Configure RegisterConfigurableComponents.h

# For each variable you want to use in C code you need to make it a 1
# or 0, because there are cases where the variable is undefined and
# you get funky code "#if PABST_FOUNT" turns into "#if " instead of
# "#if 0".
IF(PABST_FOUND)
  SET(PABST_FOUND_DEF "1" CACHE INTERNAL "Enable Pabst build")
ELSE(PABST_FOUND)
  SET(PABST_FOUND_DEF "0" CACHE INTERNAL "Disable Pabst build")
ENDIF(PABST_FOUND)

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/RegisterConfigurableComponents.h.CMakeTemplate
  ${CMAKE_BINARY_DIR}/include/RegisterConfigurableComponents.h
  )

###############################################################################
## Configure DynBVH_Parameters.h

SET(MANTA_USE_DYNBVH_PORTS OFF CACHE BOOL "Use templated DynBVH Code")
SET(MANTA_USE_DYNBVH_APPROXIMATE OFF CACHE BOOL "Use approximate DynBVH Build")
SET(MANTA_USE_DYNBVH_LAZY OFF CACHE BOOL "Use lazy DynBVH Build")

IF(MANTA_USE_DYNBVH_PORTS)
  SET(MANTA_USE_DYNBVH_PORTS_DEF "1")
ELSE(MANTA_USE_DYNBVH_PORTS)
  SET(MANTA_USE_DYNBVH_PORTS_DEF "0")
ENDIF(MANTA_USE_DYNBVH_PORTS)

IF(MANTA_USE_DYNBVH_APPROXIMATE)
  SET(MANTA_USE_DYNBVH_APPROXIMATE_DEF "1")
ELSE(MANTA_USE_DYNBVH_APPROXIMATE)
  SET(MANTA_USE_DYNBVH_APPROXIMATE_DEF "0")
ENDIF(MANTA_USE_DYNBVH_APPROXIMATE)

IF(MANTA_USE_DYNBVH_LAZY)
  SET(MANTA_USE_DYNBVH_LAZY_DEF "1")
ELSE(MANTA_USE_DYNBVH_LAZY)
  SET(MANTA_USE_DYNBVH_LAZY_DEF "0")
ENDIF(MANTA_USE_DYNBVH_LAZY)

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/DynBVH_Parameters.h.CMakeTemplate
  ${CMAKE_BINARY_DIR}/include/DynBVH_Parameters.h
  )

# Configure the MachineParameters.h

# Default to off
SET(MANTA_X86_DEF "0")

IF (CMAKE_SYSTEM_PROCESSOR MATCHES "i686" OR
    CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
  SET(MANTA_X86_DEF "1")
ENDIF(CMAKE_SYSTEM_PROCESSOR MATCHES "i686" OR
      CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")

# This takes advantage of the previously run
# Manta/CMake/ConfigArchitecture.cmake
IF(APPLE_X86)
  SET(MANTA_X86_DEF "1")
ENDIF(APPLE_X86)

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/MachineParameters.h.CmakeTemplate
  ${CMAKE_BINARY_DIR}/include/MachineParameters.h
  )

###############################################################################
## Configure Parameters.h formerly Interface/Parameters.h
SET(MANTA_MAXCACHELINESIZE 128   CACHE STRING "CPU cache line size in bytes.")
SET(MANTA_T_EPSILON        1.e-4 CACHE STRING "Epsilon parameter used for intersection.")
SET(MANTA_DENORM_EPSILON   1.e-6 CACHE STRING "Epsilon parameter used for .")
SET(MANTA_COLOR_EPSILON    1.e-4 CACHE STRING "Epsilon parameter used for .")
SET(MANTA_MAXT             1.e19 CACHE STRING "Initial intersection t value")
SET(MANTA_IMAGE_ROW_BYTES  32    CACHE STRING "Framebuffer row padding alignment.")

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/Parameters.h.CMakeTemplate
  ${CMAKE_BINARY_DIR}/include/Parameters.h
  )
