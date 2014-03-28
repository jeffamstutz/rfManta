

# Appends VAL to the string contained in STR
MACRO(APPEND_TO_STRING STR VAL)
  # You need to double ${} STR to get the value.  The first one gets
  # the variable, the second one gets the value.
  SET(${STR} "${${STR}} ${VAL}")
ENDMACRO(APPEND_TO_STRING)

# Prepends VAL to the string contained in STR
MACRO(PREPEND_TO_STRING STR VAL)
  # You need to double ${} STR to get the value.  The first one gets
  # the variable, the second one gets the value.
  SET(${STR} "${VAL} ${${STR}}")
ENDMACRO(PREPEND_TO_STRING)


#################################################################
#  FORCE_ADD_FLAGS(parameter flags)
#
# This will add arguments not found in ${parameter} to the end.  It
# does not attempt to remove duplicate arguments already existing in
# ${parameter}.
#################################################################
MACRO(FORCE_ADD_FLAGS parameter)
  # Create a separated list of the arguments to loop over
  SET(p_list ${${parameter}})
  SEPARATE_ARGUMENTS(p_list)
  # Make a copy of the current arguments in ${parameter}
  SET(new_parameter ${${parameter}})
  # Now loop over each required argument and see if it is in our
  # current list of arguments.
  FOREACH(required_arg ${ARGN})
    # This helps when we get arguments to the function that are
    # grouped as a string:
    #
    # ["-msse -msse2"]  instead of [-msse -msse2]
    SET(TMP ${required_arg}) #elsewise the Seperate command doesn't work)
    SEPARATE_ARGUMENTS(TMP)
    FOREACH(option ${TMP})
      # Look for the required argument in our list of existing arguments
      SET(found FALSE)
      FOREACH(p_arg ${p_list})
        IF (${p_arg} STREQUAL ${option})
          SET(found TRUE)
        ENDIF (${p_arg} STREQUAL ${option})
      ENDFOREACH(p_arg)
      IF(NOT found)
        # The required argument wasn't found, so we need to add it in.
        SET(new_parameter "${new_parameter} ${option}")
      ENDIF(NOT found)
    ENDFOREACH(option ${TMP})
  ENDFOREACH(required_arg ${ARGN})
  SET(${parameter} ${new_parameter} CACHE STRING "" FORCE)
ENDMACRO(FORCE_ADD_FLAGS)

# This MACRO is designed to set variables to default values only on
# the first configure.  Subsequent configures will produce no ops.
MACRO(FIRST_TIME_SET VARIABLE VALUE TYPE COMMENT)
  IF(NOT PASSED_FIRST_CONFIGURE)
    SET(${VARIABLE} ${VALUE} CACHE ${TYPE} ${COMMENT} FORCE)
  ENDIF(NOT PASSED_FIRST_CONFIGURE)
ENDMACRO(FIRST_TIME_SET)

MACRO(FIRST_TIME_MESSAGE STR )
  IF(NOT PASSED_FIRST_CONFIGURE)
    MESSAGE(${STR})
  ENDIF(NOT PASSED_FIRST_CONFIGURE)  
ENDMACRO(FIRST_TIME_MESSAGE)

MACRO(SUBDIRS_IF VARIABLE DESCRIPTION DIRS)
  # Add the cached variable.
  SET(${VARIABLE} 0 CACHE BOOL ${DESCRIPTION})
  IF(${VARIABLE})
    SUBDIRS(${DIRS})
  ENDIF(${VARIABLE})
ENDMACRO(SUBDIRS_IF)

SET(MANTA_THREE_PART_VERSION_REGEX "[0-9]+\\.[0-9]+\\.[0-9]+")
# Computes the realtionship between two version strings.  A version
# string is a number delineated by '.'s such as 1.3.2 and 0.99.9.1.
# You can feed version strings with different number of dot versions,
# and the shorter version number will be padded with zeros: 9.2 <
# 9.2.1 will actually compare 9.2.0 < 9.2.1.
#
# Input: a_in - value, not variable
#        b_in - value, not variable
#        result_out - variable with value:
#                         -1 : a_in <  b_in
#                          0 : a_in == b_in
#                          1 : a_in >  b_in
#
# Written by James Bigler.
MACRO(COMPARE_VERSION_STRINGS a_in b_in result_out)
  # Since SEPARATE_ARGUMENTS using ' ' as the separation token,
  # replace '.' with ' ' to allow easy tokenization of the string.
  STRING(REPLACE "." " " a ${a_in})
  STRING(REPLACE "." " " b ${b_in})
  SEPARATE_ARGUMENTS(a)
  SEPARATE_ARGUMENTS(b)

  # Check the size of each list to see if they are equal.
  LIST(LENGTH a a_length)
  LIST(LENGTH b b_length)

  # Pad the shorter list with zeros.
  
  # Note that range needs to be one less than the length as the for
  # loop is inclusive (silly CMake).
  IF(a_length LESS b_length)
    # a is shorter
    SET(shorter a)
    MATH(EXPR range "${b_length} - 1")
    MATH(EXPR pad_range "${b_length} - ${a_length} - 1")
  ELSE(a_length LESS b_length)
    # b is shorter
    SET(shorter b)
    MATH(EXPR range "${a_length} - 1")
    MATH(EXPR pad_range "${a_length} - ${b_length} - 1")
  ENDIF(a_length LESS b_length)

  # PAD out if we need to
  IF(NOT pad_range LESS 0)
    FOREACH(pad RANGE ${pad_range})
      # Since shorter is an alias for b, we need to get to it by by dereferencing shorter.
      LIST(APPEND ${shorter} 0)
    ENDFOREACH(pad RANGE ${pad_range})
  ENDIF(NOT pad_range LESS 0)

  SET(result 0)
  FOREACH(index RANGE ${range})
    IF(result EQUAL 0)
      # Only continue to compare things as long as they are equal
      LIST(GET a ${index} a_version)
      LIST(GET b ${index} b_version)
      # LESS
      IF(a_version LESS b_version)
        SET(result -1)
      ENDIF(a_version LESS b_version)
      # GREATER
      IF(a_version GREATER b_version)
        SET(result 1)
      ENDIF(a_version GREATER b_version)
    ENDIF(result EQUAL 0)
  ENDFOREACH(index)

  # Copy out the return result
  SET(${result_out} ${result})
ENDMACRO(COMPARE_VERSION_STRINGS)

# Only adds a test if TEST_NUM_PROCS is >= NP.  !(x < NP) == (x >= NP)
# equavelance is used because CMake doesn't have >= test.
MACRO(ADD_NP_TEST NP)
  IF(NOT TEST_NUM_PROCS LESS ${NP})
    ADD_TEST(${ARGN})
  ENDIF(NOT TEST_NUM_PROCS LESS ${NP})
ENDMACRO(ADD_NP_TEST)
