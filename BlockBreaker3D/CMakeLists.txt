find_package(SDL3 REQUIRED)

file(GLOB_RECURSE GAME_SRC src/*.cpp src/*.h)

add_executable(${PROJECT_NAME} ${GAME_SRC})

#target_include_directories(${PROJECT_NAME} PRIVATE ${SDL3_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)