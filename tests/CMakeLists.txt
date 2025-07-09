# tests/CMakeLists.txt
if (NOT RUN_TESTS)
  return()
endif()

# 1) Gather all C test modules (files matching test_*.c)
file(GLOB TEST_C_MODULES
  CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/test_*.c
)

# 2) Gather all C++ test modules (files matching test_*.cpp)
file(GLOB TEST_CPP_MODULES
  CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp
)

# 3) Combine both C and C++ test modules
set(TEST_MODULES
  ${TEST_C_MODULES}
  ${TEST_CPP_MODULES}
)

# 4) Specify the single test runner (provides main())
#    Written in C++ so it can link both C and C++ tests
set(RUNNER ${CMAKE_CURRENT_SOURCE_DIR}/test_runner.cpp)

# 5) Create the unified test executable
add_executable(test_all
  ${RUNNER}
  ${TEST_MODULES}
)

# 6) Include directories for your production headers and Unity
target_include_directories(test_all PRIVATE
  ${CMAKE_SOURCE_DIR}/include       # production API headers
  ${unity_SOURCE_DIR}/src           # Unity framework headers
)

# 7) Link against Unity and the CoreLib library containing your logic
target_link_libraries(test_all PRIVATE
  Unity::Unity
  CoreLib
)

# 8) Register the single test with CTest
add_test(NAME test_all COMMAND test_all)
