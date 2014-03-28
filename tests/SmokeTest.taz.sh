# Add this to the crontab
#
#  1  02   *   *   * bash /Users/bigler/manta/testing/src/tests/SmokeTest.taz.sh >> /Users/bigler/manta/testing/logs/tests.log 2>&1
#

# Get the path to the script directory
TEST_SCRIPT_PATH=`dirname $0`
source $TEST_SCRIPT_PATH/common.sh

start_time;

#######################################################
# Set parameters.  Set these before calling setup below

#########
# These ones will figure them selves out, but you can override them here

# Set this if cmake isn't in your default path
PATH_TO_CMAKE=/opt/local/bin

# Path to manta source tree
#MANTA_SRC_PATH=/home/sci/bigler/manta/testing/src

# Path to where to write the script logs
#MANTA_LOG_PATH=${MANTA_SRC_PATH}/logs

#######
# These ones should always be set, but their values can be overriden from the command line

# Number of cores to run the tests on
NP=8
# Number of threads to use for a parallel build
MAKE_PARALLEL=12
# Type of test, can be Experimental or Nightly (or something supported by ctest)
TEST_TYPE=Nightly
# Weather or not to do a clean build
CLEAN_BUILD=ON


# Make sure you pass in the script arguments '$@'
setup $@;

run_sse_test;
run_nosse_float_test;
run_nosse_double_test;

finish_time;
