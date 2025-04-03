find_package(SDL3 REQUIRED)
find_package(glm REQUIRED)

file(GLOB_RECURSE GAME_SRC src/*.cpp src/*.h)

add_executable(${PROJECT_NAME} ${GAME_SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 glm::glm)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>" $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND_EXPAND_LISTS
)