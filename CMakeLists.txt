# --- Initial Project Setup ---

cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(
  LogisticsServer
  VERSION 1.0.0
  DESCRIPTION "Distributed Logistics Simulation Server"
  LANGUAGES C CXX
)

# ---------------------------------------------------------
#  Language settings
# ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# ---------------------------------------------------------
#  Unity for tests
# ---------------------------------------------------------
include(FetchContent)
include(CTest)

# ---------------------------------------------------------
#  QUality: fmt, clang-tidy, sanitizers, coverage, docs
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



# --- [IMPORTANT] FHS Structure Definition ---
# All executables will be placed in the 'bin' folder.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# All libraries (.dll, .so, .a) will be placed in the 'lib' folder.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # for .a libs


# ---------------------------------------------------------
# ServerLib and ClientLib
# ---------------------------------------------------------
# 1. Server-side logic library
add_library(ServerLib STATIC
    src/server/server.cpp
    src/server/commandProcessor.cpp
    src/server/argsParser.cpp
    # ... add other server modules here
)
# Make headers from 'include/server' available
target_include_directories(ServerLib PUBLIC
  ${CMAKE_SOURCE_DIR}/include/server
)

# 2. Client-side logic library (as a shared library .so)
add_library(ClientLib SHARED
    src/client/client.c
    #src/client/facade.c
    # ... add other client logic files here
)
target_include_directories(ClientLib PUBLIC
  ${CMAKE_SOURCE_DIR}/include/client)


# ---------------------------------------------------------
#   Executable Definitions
# ---------------------------------------------------------
# 1. Server Executable
# The source file only contains the main() function
add_executable(server src/server/main.cpp)

# Link the server executable against its logic library
target_link_libraries(server PRIVATE ServerLib)

# 2. Client Executable
# This source file also only contains main()
add_executable(client src/client/main.c)

# Link the client executable against its logic library
target_link_libraries(client PRIVATE ClientLib)

# ---------------------------------------------------------
#  tests
# ---------------------------------------------------------
enable_testing()

option(RUN_TESTS "Enable building and running unit tests" ON)
if(RUN_TESTS)
  FetchContent_Declare(
    Unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/UNITY_BUILD
    UPDATE_COMMAND ""
  )
  FetchContent_MakeAvailable(Unity) #fetchcontent creates and links the library automaticly
endif()

if(RUN_TESTS)
  add_subdirectory(tests)
endif()
