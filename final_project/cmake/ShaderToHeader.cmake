# ShaderToHeader.cmake
# Converts shader files to C++ header with string constants

function(shaders_to_header OUTPUT_FILE)
    set(SHADER_FILES ${ARGN})

    # Start generating header content
    set(HEADER_CONTENT "// Auto-generated shader implementation\n")
    set(HEADER_CONTENT "${HEADER_CONTENT}// Do not edit manually\n\n")
    set(HEADER_CONTENT "${HEADER_CONTENT}#pragma once\n\n")
    set(HEADER_CONTENT "${HEADER_CONTENT}namespace shaders {\n\n")

    foreach(SHADER_FILE ${SHADER_FILES})
        message(STATUS "Processing shader: ${SHADER_FILE}")

        # Get the file name without path and extension
        get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)
        get_filename_component(SHADER_EXT ${SHADER_FILE} EXT)

        # Remove the leading dot from extension
        string(SUBSTRING ${SHADER_EXT} 1 -1 SHADER_EXT_CLEAN)

        # Convert to uppercase for constant name: ocean.vert -> OCEAN_VERT_SHADER
        string(TOUPPER "${SHADER_NAME}_${SHADER_EXT_CLEAN}_SHADER" CONSTANT_NAME)
        message(STATUS "  -> ${CONSTANT_NAME}")

        # Read shader file content
        file(READ ${SHADER_FILE} SHADER_CONTENT)

        # Ensure content ends with newline, then remove all trailing newlines
        string(REGEX REPLACE "\n+$" "" SHADER_CONTENT "${SHADER_CONTENT}")

        # Escape special characters for C++ string literal
        string(REPLACE "\\" "\\\\" SHADER_CONTENT "${SHADER_CONTENT}")
        string(REPLACE "\"" "\\\"" SHADER_CONTENT "${SHADER_CONTENT}")
        string(REPLACE "\n" "\\n\"\n    \"" SHADER_CONTENT "${SHADER_CONTENT}")

        # Add constant definition to header with explicit newline at end
        set(HEADER_CONTENT "${HEADER_CONTENT}constexpr const char* ${CONSTANT_NAME} = \n")
        set(HEADER_CONTENT "${HEADER_CONTENT}    \"${SHADER_CONTENT}\\n\";\n\n")
    endforeach()

    set(HEADER_CONTENT "${HEADER_CONTENT}} // namespace shaders\n")

    # Write the header file
    file(WRITE ${OUTPUT_FILE} "${HEADER_CONTENT}")

    message(STATUS "Generated shader header: ${OUTPUT_FILE}")
endfunction()
