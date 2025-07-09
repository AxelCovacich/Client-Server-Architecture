cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(
  LogisticsServer
  VERSION 1.0.0
  DESCRIPTION "Distributed Logistics Simulation Server"
  LANGUAGES C CXX
)

# ---------------------------------------------------------
# 1) Language settings
# ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# ---------------------------------------------------------
# 2) Unity for tests
# ---------------------------------------------------------
include(FetchContent)
include(CTest)
enable_testing()

option(RUN_TESTS "Enable building and running unit tests" ON)
if(RUN_TESTS)
  FetchContent_Declare(
    Unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/UNITY_BUILD
    UPDATE_COMMAND ""
  )
  FetchContent_MakeAvailable(Unity)

  add_library(UnityLib STATIC
    ${unity_SOURCE_DIR}/src/unity.c
  )
  target_include_directories(UnityLib PUBLIC
    ${unity_SOURCE_DIR}/src
  )
  add_library(Unity::Unity ALIAS UnityLib)
endif()

# ---------------------------------------------------------
# 3) QUality: fmt, clang-tidy, sanitizers, coverage, docs
# ---------------------------------------------------------
find_program(CLANG_FORMAT_EXE clang-format)
if(CLANG_FORMAT_EXE)
  file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS
       src/*.c src/*.cpp src/*.h src/*.hpp
  )
  add_custom_target(fmt
    COMMAND ${CLANG_FORMAT_EXE} -i --style=file ${ALL_SOURCES}
    COMMENT "Running clang-format"
  )
endif()

set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*")

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(ENABLE_ASAN)
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address)
endif()

option(ENABLE_COVERAGE "Enable coverage" OFF)
if(ENABLE_COVERAGE)
  add_compile_options(--coverage)
  add_link_options(--coverage)
endif()

find_package(Doxygen)
if(DOXYGEN_FOUND)
  add_custom_target(doc
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
    COMMENT "Generate Doxygen docs"
  )
endif()

# ---------------------------------------------------------
# 4) CoreLib: the logic of the product as a library
# ---------------------------------------------------------
add_library(CoreLib
  src/dummy.c    # and others .c and .cpp from logic, but main
  #src/leak.c
  # etc.cpp, etc.c
)
target_include_directories(CoreLib PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

# ---------------------------------------------------------
# 5) Main executable: only main.c + CoreLib
# ---------------------------------------------------------
add_executable(${PROJECT_NAME}
  src/main.c
)
target_link_libraries(${PROJECT_NAME} PRIVATE CoreLib)

# ---------------------------------------------------------
# 6) Subdirectories: src and tests
# ---------------------------------------------------------
if(RUN_TESTS)
  add_subdirectory(tests)
endif()
