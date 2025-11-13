cmake_minimum_required(VERSION 3.25)
project(ClientOnly LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Output bin in local folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../build/bin)

# for client thread
find_package(Threads REQUIRED)

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

set(PYTHON_LIB_DIR "${CMAKE_SOURCE_DIR}/../external/python_libs")

add_custom_target(
    python_deps ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Installing Python dependencies into ${PYTHON_LIB_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PYTHON_LIB_DIR}
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${PYTHON_LIB_DIR} ${Python3_EXECUTABLE} -c "import rich, posix_ipc" 2>/dev/null
        || pip install --target=${PYTHON_LIB_DIR} rich posix_ipc
    COMMENT "Checking and installing Python dependencies"
)

# Include cJSON library
add_library(cjson STATIC ../external/cJSON/cJSON.c)
target_include_directories(cjson PUBLIC ../external/cJSON)

add_library(ClientLib SHARED
    ../src/client/client.c
    ../src/client/input_handler.c
    ../src/client/transport.c
    ../src/client/session_handler.c
    ../external/cJSON/cJSON.c
    ../src/client/output_handler.c
    ../src/client/client_context.c
    ../src/client/udp_handler.c
    ../src/client/logger.c
    ../src/client/ipc_handler.c
)

target_include_directories(ClientLib PUBLIC
    ../include/client
    ../external/cJSON
)

add_executable(client ../src/client/main.c)
target_link_libraries(client PRIVATE ClientLib Threads::Threads)