# Environment configuration

set(CTEST_PROJECT_NAME "LogisticsServer")
set(CTEST_SUBMIT_URL 
    "http://localhost:8080/submit.php?project=${CTEST_PROJECT_NAME}")
set(CTEST_USE_LAUNCHERS YES)

  # General settings
  site_name(CTEST_SITE)
  set(CTEST_BUILD_NAME "linux-x86_64-gcc")
  set(CTEST_SOURCE_DIRECTORY "${CTEST_SCRIPT_DIRECTORY}")
  set(CTEST_BINARY_DIRECTORY "${CTEST_SCRIPT_DIRECTORY}/build")
  
  # Exclude third-party libs and our own tests from the coverage report
  list(APPEND CTEST_CUSTOM_COVERAGE_EXCLUDE
  "/external/"
  "/tests/"
  )

  # Build settings
  set(CTEST_CMAKE_GENERATOR Ninja)
  set(CTEST_CONFIGURATION_TYPE Debug)
  set(configureOpts
    "-DENABLE_COVERAGE=ON"
    "-DRUN_TESTS=ON"
    "-DCMAKE_BUILD_TYPE=Debug"
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
  )   
  # Coverage settings
  set(CTEST_COVERAGE_COMMAND gcov)

#Memcheck via valgrind
find_program(CTEST_MEMORYCHECK_COMMAND valgrind)
set(CTEST_MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full --error-exitcode=1 --xml=yes --xml-file=${CTEST_BINARY_DIRECTORY}/valgrind_report.xml")

# Step 1 - Clean previous pipeline run
ctest_empty_binary_directory(${CTEST_BINARY_DIRECTORY})

# Step 2 - Configure
ctest_start(Experimental)
ctest_configure(OPTIONS "${configureOpts}")
ctest_submit(PARTS Start Configure)

# Step 3 - Build
ctest_build(PARALLEL_LEVEL 16)
ctest_submit(PARTS Build)

# Step 4 - Run unit tests
ctest_test(PARALLEL_LEVEL 16)
ctest_submit(PARTS Test)

# Step 5 - Mem leak     
ctest_memcheck()
ctest_submit(PARTS Memcheck)

# Step 5 - Collect coverage information
ctest_coverage()
ctest_submit(PARTS Coverage Done)