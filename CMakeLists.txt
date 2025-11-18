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

option(ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
if(ENABLE_CLANG_TIDY)
  set(CMAKE_CXX_CLANG_TIDY
      "clang-tidy;-checks=*;-header-filter=^${CMAKE_SOURCE_DIR}/(src|include)/.*$")
endif()

option(ENABLE_ASAN "Enable AddressSanitizer" ON)
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

# for client thread
find_package(Threads REQUIRED)

option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
if(ENABLE_TSAN)
  add_compile_options(-fsanitize=thread)
  add_link_options(-fsanitize=thread)
endif()

# --- [IMPORTANT] FHS Structure Definition ---
# All executables will be placed in the 'bin' folder.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# All libraries (.dll, .so, .a) will be placed in the 'lib' folder.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # for .a libs


# --- Dependencies ---
find_package(ZLIB REQUIRED)

# --- YAML Configuration Library ---

FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  #GIT_TAG yaml-cpp-0.8.0
)
set(YAML_CPP_BUILD_TESTS OFF)

FetchContent_MakeAvailable(yaml-cpp)

# ---- Prometheus-cpp -----
find_package(prometheus-cpp REQUIRED)

# ---- JSON Library ----
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json
  GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(json)


# --- SQLiteCpp Library ---
FetchContent_Declare(
  sqlitecpp
  GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
  GIT_TAG 3.3.0
  # download and compile sqlite3
  CMAKE_ARGS "-DSQLITECPP_INTERNAL_SQLITE=ON"
)

FetchContent_MakeAvailable(sqlitecpp)


# --- Bcrypt library ---
FetchContent_Declare(
  bcrypt
  GIT_REPOSITORY https://github.com/hilch/Bcrypt.cpp.git
  #GIT_TAG
)
set(BCRYPT_TESTS OFF)

FetchContent_MakeAvailable(bcrypt)

# --- Python dependencies for dashboard ---

find_package(Python3 REQUIRED COMPONENTS Interpreter)

execute_process(
    COMMAND ${Python3_EXECUTABLE} -m pip --version
    RESULT_VARIABLE PIP_CHECK
    OUTPUT_QUIET
    ERROR_QUIET
)

if (NOT PIP_CHECK EQUAL 0)
    message(FATAL_ERROR "pip is not available for ${Python3_EXECUTABLE}. Please install pip.")
endif()

set(PYTHON_LIB_DIR "${CMAKE_SOURCE_DIR}/external/python_libs")

add_custom_target(
    python_deps ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Installing Python dependencies into ${PYTHON_LIB_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PYTHON_LIB_DIR}
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${PYTHON_LIB_DIR} ${Python3_EXECUTABLE} -c "import rich, posix_ipc" 2>/dev/null
        || pip install --target=${PYTHON_LIB_DIR} rich posix_ipc
    COMMENT "Checking and installing Python dependencies"
)

# --- Library Definitions ---

# 1. ServerLib
add_library(ServerLib STATIC)

# Add the library's source files
target_sources(ServerLib PRIVATE
    src/server/server.cpp
    src/server/commandProcessor.cpp
    src/server/inventory.cpp
    src/server/authenticator.cpp
    src/server/clientSession.cpp
    src/server/storage.cpp
    src/server/logger.cpp
    src/server/alertManager.cpp
    src/server/sessionManager.cpp
    src/server/config.cpp
    src/server/configValidator.cpp
    src/server/udpHandler.cpp
    src/server/ipcHandler.cpp
    src/server/trafficReporter.cpp
    src/server/eventQueue.cpp
    src/server/threadPool.cpp
)

# Add the path to ServerLib's PUBLIC headers
target_include_directories(ServerLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/server
)

target_link_libraries(ServerLib PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(ServerLib PUBLIC SQLiteCpp bcrypt yaml-cpp)
target_link_libraries(ServerLib PRIVATE ZLIB::ZLIB)
if(TARGET prometheus-cpp::core AND TARGET prometheus-cpp::pull)
  message(STATUS "prometheus-cpp targets found. Linking to ServerLib")
  target_link_libraries(ServerLib PUBLIC prometheus-cpp::core prometheus-cpp::pull)
else()
  message(WARNING "prometheus-cpp targets NOT found.")
endif()

target_compile_options(ServerLib PRIVATE -Wall -Wextra -Wpedantic -Wswitch-enum -Wshadow -Wno-long-long)


# 2. ClientLib
add_library(ClientLib SHARED
    src/client/client.c
    src/client/input_handler.c
    external/cJSON/cJSON.c
    src/client/transport.c
    src/client/session_handler.c
    src/client/output_handler.c
    src/client/client_context.c
    src/client/udp_handler.c
    src/client/logger.c
    src/client/ipc_handler.c
)
target_include_directories(ClientLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/client
    ${CMAKE_CURRENT_SOURCE_DIR}/external/cJSON
)

target_compile_options(ClientLib PRIVATE -Wall -Wextra -Wpedantic -Wswitch-enum -Wshadow -Wno-long-long)

# --- Executable Definitions ---

# 1. Server Executable
add_executable(server src/server/main.cpp)

target_link_libraries(server PRIVATE ServerLib)


# 2. Client Executable
add_executable(client src/client/main.c)
target_link_libraries(client PRIVATE ClientLib Threads::Threads)
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

get_target_property(SERVER_LIB_INCLUDES ServerLib INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "Include Dirs for ServerLib: ${SERVER_LIB_INCLUDES}")