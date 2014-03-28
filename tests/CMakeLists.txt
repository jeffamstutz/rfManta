
SUBDIRS( perftest )

ADD_EXECUTABLE(atomic_counter atomic_counter.cc)
TARGET_LINK_LIBRARIES(atomic_counter ${MANTA_TARGET_LINK_LIBRARIES})

IF(BUILD_TESTING)
  SET(AtomicIterations 100)

  ADD_NP_TEST(1 AtomicCounter_NP1 ${CMAKE_BINARY_DIR}/bin/atomic_counter 1 ${AtomicIterations})
  ADD_NP_TEST(2 AtomicCounter_NP2 ${CMAKE_BINARY_DIR}/bin/atomic_counter 2 ${AtomicIterations})
  ADD_NP_TEST(4 AtomicCounter_NP4 ${CMAKE_BINARY_DIR}/bin/atomic_counter 4 ${AtomicIterations})
  ADD_NP_TEST(8 AtomicCounter_NP8 ${CMAKE_BINARY_DIR}/bin/atomic_counter 8 ${AtomicIterations})
ENDIF(BUILD_TESTING)
