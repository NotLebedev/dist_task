cmake_minimum_required(VERSION 3.20)
project(dist_task)

set(CMAKE_CXX_STANDARD 20)

# Add MPI Package to Project
find_package(MPI REQUIRED)

# Add toml parsing library
include(FetchContent)
FetchContent_Declare(
        tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG        v3.2.0
)
FetchContent_MakeAvailable(tomlplusplus)

add_executable(dist_task main.cpp Client.cpp Client.h Server.cpp Server.h Commands.h Commands.cpp Commands.cpp MPIHelper.h MPIHelper.cpp)
# Add libraries for code completion and compiling
target_link_libraries(dist_task PUBLIC MPI::MPI_CXX PUBLIC tomlplusplus::tomlplusplus)
