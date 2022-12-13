cmake_minimum_required(VERSION 3.10)
project(voting_protocol)

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

add_executable(voting_protocol main.cpp Client.cpp Client.h Server.cpp Server.h Commands.h Commands.cpp Commands.cpp MPIHelper.h MPIHelper.cpp)
# Add libraries for code completion and compiling
target_link_libraries(voting_protocol PUBLIC MPI::MPI_CXX PUBLIC tomlplusplus::tomlplusplus)
