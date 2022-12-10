cmake_minimum_required(VERSION 3.20)
project(dist_task)

set(CMAKE_CXX_STANDARD 20)

# Add MPI Package to Project
find_package(MPI REQUIRED)

add_executable(dist_task main.cpp)
# Add libraries for code completion and compiling
target_link_libraries(dist_task PUBLIC MPI::MPI_CXX)
