# Shader Compilation
set(SHADER_DIR "${CMAKE_SOURCE_DIR}/Shaders")
set(SPIRV_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/Shaders")

file(GLOB_RECURSE GLSL_FILES
		"${SHADER_DIR}/*.vert"
		"${SHADER_DIR}/*.frag"
		"${SHADER_DIR}/*.comp"
)

set(SPIRV_FILES)

foreach(SHADER ${GLSL_FILES})
	get_filename_component(SHADER_NAME ${SHADER} NAME)

	# Output path
	set(SPIRV_FILE "${SPIRV_DIR}/${SHADER_NAME}.spv")

	add_custom_command(
		OUTPUT ${SPIRV_FILE}
		COMMAND $ENV{VULKAN_SDK}/Bin/glslangValidator.exe
		ARGS -V ${SHADER} -o ${SPIRV_FILE}
		DEPENDS ${SHADER}
		COMMENT "Compiling GLSL shader: ${SHADER_NAME}.glsl -> ${SHADER_NAME}.spv"
		VERBATIM
	)

	list(APPEND SPIRV_FILES ${SPIRV_FILE})
endforeach()

add_custom_target(
    Shaders
    DEPENDS ${SPIRV_FILES}
    COMMENT "Building all shaders"
)