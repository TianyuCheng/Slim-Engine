function(add_slim_project)
    # parse args
    set(prefix SLIM_PROJECT)
    set(flags "")
    set(singleValues TARGET SPV)
    set(multiValues SOURCES SHADERS)
    include(CMakeParseArguments)
    cmake_parse_arguments(${prefix}
                         "${flags}"
                         "${singleValues}"
                         "${multiValues}"
                          ${ARGN})

    find_program(CMAKE_GLSL_COMPILER glslc REQUIRED)
    mark_as_advanced(CMAKE_GLSL_COMPILER)

    # generate shader objects
    foreach(SHADER ${SLIM_PROJECT_SHADERS})
        get_filename_component(FILENAME  ${SHADER} NAME)
        get_filename_component(DIRECTORY ${SHADER} DIRECTORY)
        file(MAKE_DIRECTORY ${DIRECTORY})
        set(INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY}/${FILENAME}")
        set(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${DIRECTORY}/${FILENAME}.spv")
        set(DEPFILE "${CMAKE_CURRENT_BINARY_DIR}/${DIRECTORY}/${FILENAME}.spv.d")
        set(INCLUDE "${PROJECT_SOURCE_DIR}/Library/shaderlib")
        add_custom_command(
            OUTPUT  ${OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/${DIRECTORY}/"
            COMMAND ${CMAKE_GLSL_COMPILER} -MD --target-env=${SLIM_PROJECT_SPV} -I ${INCLUDE} ${INPUT} -g -O -o ${OUTPUT}
            DEPENDS ${INPUT}
            # DEPFILE ${DEPFILE} # This is not supported by Visual Studio 2019
            COMMENT "Building shader: ${OUTPUT}"
        )
        list(APPEND SPIRV_BINARY_FILES ${OUTPUT})
    endforeach(SHADER)

    # generate project target with both sources and shaders
    add_executable(${SLIM_PROJECT_TARGET}
                   ${SLIM_PROJECT_SOURCES}
                   ${SPIRV_BINARY_FILES})
    target_link_libraries(${SLIM_PROJECT_TARGET} PRIVATE slim)
endfunction()
