# This defines some functions common to all the test scripts

start_time()
{
    echo "#############################################"
    echo "Started  "`date`
    echo "#"
}

finish_time()
{
    echo "#"
    echo "Finished "`date`
}

setup()
{
    while [ "$1" != "" ]; do
        case "$1" in
            --experimental)
                echo "doing experimental build";
                TEST_TYPE=Experimental;;
            --nightly)
                echo "doing nightly build";
                TEST_TYPE=Nightly;;
            --cleanbuild)
                echo "cleaning the build before running test";
                CLEAN_BUILD=ON;;
            --nocleanbuild)
                echo "not cleaning the build before running test";
                CLEAN_BUILD=OFF;;
            --makeparallel)
                shift 1;
                MAKE_PARALLEL=$1;
                echo "Using $MAKE_PARALLEL tread(s) for compilation";;
            --np)
                shift 1;
                NP=$1;
                echo "Testing with $NP core(s)";;
            *)
                echo "unrecognized argument ${1}";;
        esac
        shift 1
    done

  # Add the path to cmake to the list if specified
    if [[ -d $PATH_TO_CMAKE ]]; then
        echo "adding $PATH_TO_CMAKE to the path"
        export PATH=$PATH_TO_CMAKE:$PATH;
        echo "PATH = $PATH"
    fi
    
  # Set the MANTA_SRC_PATH
    if [[ ! $MANTA_SRC_PATH ]]; then
        MANTA_SRC_PATH=$TEST_SCRIPT_PATH/..;
        echo "MANTA_SRC_PATH not found.  Setting to $MANTA_SRC_PATH";
    fi

    echo "MANTA_SRC_PATH = $MANTA_SRC_PATH"
    
    if [[ ! $MANTA_LOG_PATH ]]; then
        MANTA_LOG_PATH=$MANTA_SRC_PATH/logs
        echo "MANTA_LOG_PATH not found.  Setting to $MANTA_LOG_PATH";
    fi
    
  # Make sure the log directory exists
    if [[ ! -d ${MANTA_LOG_PATH} ]]; then
        mkdir ${MANTA_LOG_PATH};
    fi
}

run_test()
{
    if [[ $# != 3 ]]; then
        echo "Wrong number of arguments passed to $0.  Wanted 3 and got $#"
        return 1;
    fi
    
    test_name=$1
    enable_sse=$2
    real_type=$3
    script_name=${MANTA_SRC_PATH}/tests/`hostname`.$test_name.cmake
    echo "#"
    echo "Starting $test_name test: "`date`

    cmake \
        -DBUILD_DIR:STRING=build-ctest-$test_name \
        -DCLEAN_BUILD:BOOL=$CLEAN_BUILD \
        -DTEST_TYPE:STRING=$TEST_TYPE \
        -DNUM_CORES:INTEGER=$NP \
        -DMAKE_PARALLEL:INTEGER=$MAKE_PARALLEL \
        -DENABLE_SSE:BOOL=$enable_sse \
        -DREAL_TYPE:STRING=$real_type \
        -DSCRIPT_NAME:STRING=$script_name \
        -P ${MANTA_SRC_PATH}/tests/GenerateTest.cmake

    ctest -S $script_name -V >> ${MANTA_LOG_PATH}/`date +%F`.log 2>&1
}

run_sse_test()
{
    run_test "sse" "TRUE" "float"
}

run_nosse_float_test()
{
    run_test "nosse-float" "FALSE" "float"
}

run_nosse_double_test()
{
    run_test "nosse-double" "FALSE" "double"
}
