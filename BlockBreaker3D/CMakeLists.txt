find_package(SDL3 REQUIRED)
find_package(glm REQUIRED)
find_package(assimp REQUIRED)
find_package(freetype REQUIRED)

file(GLOB_RECURSE GAME_SRC src/*.cpp src/*.h src/*.c)

add_executable(${PROJECT_NAME} ${GAME_SRC})

target_include_directories(${PROJECT_NAME} PRIVATE "$ENV{C-LIBS}/stb")
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 glm::glm assimp::assimp Freetype::Freetype)

add_custom_command(
    TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${CMAKE_CURRENT_BINARY_DIR}/assets"
    COMMENT "Copying assets to binary directory"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>" $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND_EXPAND_LISTS
)