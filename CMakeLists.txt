cmake_minimum_required(VERSION 3.15)
project(BlockBreaker3D)

add_subdirectory(Shaders)
add_subdirectory(BlockBreaker3D)
add_dependencies(${PROJECT_NAME} Shaders)