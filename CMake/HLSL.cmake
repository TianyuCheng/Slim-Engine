function(add_hlsl)

    # Find glslc compiler.
    find_program(CMAKE_HLSL_COMPILER glslc REQUIRED)
    mark_as_advanced(CMAKE_HLSL_COMPILER)

    # Find spirv linker.
    find_program(CMAKE_HLSL_LINKER spirv-link REQUIRED)
    mark_as_advanced(CMAKE_HLSL_LINKER)

    # Parse arguments.
    set(prefix ADD_HLSL)
    set(flags "")
    set(singleValues OUTPUT VERSION SOURCE COMPUTE VERTEX FRAGMENT GEOMETRY TESSCTRL TESSEVAL RAYGEN RAYCHIT RAYAHIT RAYMISS)
    set(multiValues INCLUDE)
    include (CMakeParseArguments)
    cmake_parse_arguments(${prefix} "${flags}" "${singleValues}" "${multiValues}" ${ARGN})

    # Prepare directory
    make_directory(${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${ADD_HLSL_OUTPUT}.dir)

    # Create common variables
    set(ADD_HLSL_COMPILE_FLAGS -g -O -Os)
    set(ADD_HLSL_OBJECT_DIR "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${ADD_HLSL_OUTPUT}.dir")
    set(ADD_HLSL_OBJECTS "")

    # Prepend -I for each include directory
    set(ADD_HLSL_INCLUDE_DIRS "")
    foreach(INCLUDE ${ADD_HLSL_INCLUDE})
        get_filename_component(ABS_INCLUDE ${INCLUDE} ABSOLUTE)
        list(APPEND ADD_HLSL_INCLUDE_DIRS -I${ABS_INCLUDE})
    endforeach()

    # NOTE: This is a bit crude, because most of the generators does not support DEPFILE.
    # Therefore we scan the whole include directories to scan for any possible dependencies.
    set(ADD_HLSL_INCLUDE_DEPS "")
    foreach(INCLUDE ${ADD_HLSL_INCLUDE})
        get_filename_component(ABS_INCLUDE ${INCLUDE} ABSOLUTE)
        file(GLOB_RECURSE FILES ${ABS_INCLUDE}/*.hlsli)
        list(APPEND ADD_HLSL_INCLUDE_DEPS ${FILES})
    endforeach()

    # Define sub-targets for the final target.
    # Each sub-target is a shader spirv file with entry.
    # All different shader stages use the same compiling logic, so we define a macro.
    macro(add_hlsl_subtarget STAGE ENTRY)
        set(ADD_HLSL_SUBTARGET "${ADD_HLSL_OBJECT_DIR}/${ENTRY}.spv")
        list(APPEND ADD_HLSL_OBJECTS ${ADD_HLSL_SUBTARGET})
        get_filename_component(SOURCE ${ADD_HLSL_SOURCE} ABSOLUTE)
        add_custom_command(
            OUTPUT  ${ADD_HLSL_SUBTARGET}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ADD_HLSL_OBJECT_DIR}
            COMMAND ${CMAKE_HLSL_COMPILER} -MD --target-env=${ADD_HLSL_VERSION}
                                                -fshader-stage=${STAGE}
                                                -fentry-point=${ENTRY}
                                                ${ADD_HLSL_INCLUDE_DIRS}
                                                ${SOURCE}
                                                ${ADD_HLSL_COMPILE_FLAGS} -o ${ADD_HLSL_SUBTARGET}
            MAIN_DEPENDENCY ${SOURCE}
            DEPENDS ${ADD_HLSL_INCLUDE_DEPS}
            WORKING_DIRECTORY ${ADD_HLSL_OBJECT_DIR}
            BYPRODUCTS ${ADD_HLSL_SUBTARGET}.d)
    endmacro()

    # Add compute shaders.
    foreach(ENTRY ${ADD_HLSL_COMPUTE})
        add_hlsl_subtarget(compute ${ENTRY})
    endforeach()

    # Add vertex shaders.
    foreach(ENTRY ${ADD_HLSL_VERTEX})
        add_hlsl_subtarget(vertex ${ENTRY})
    endforeach()

    # Add fragment shaders.
    foreach(ENTRY ${ADD_HLSL_FRAGMENT})
        add_hlsl_subtarget(fragment ${ENTRY})
    endforeach()

    # Add geometry shaders.
    foreach(ENTRY ${ADD_HLSL_GEOMETRY})
        add_hlsl_subtarget(geometry ${ENTRY})
    endforeach()

    # Add tessctrl shaders.
    foreach(ENTRY ${ADD_HLSL_TESSCTRL})
        add_hlsl_subtarget(tesscontrol ${ENTRY})
    endforeach()

    # Add tesseval shaders.
    foreach(ENTRY ${ADD_HLSL_TESSEVAL})
        add_hlsl_subtarget(tesseval ${ENTRY})
    endforeach()

    # Add rgen shaders.
    foreach(ENTRY ${ADD_HLSL_RAYGEN})
        add_hlsl_subtarget(rgen ${ENTRY})
    endforeach()

    # Add rchit shaders.
    foreach(ENTRY ${ADD_HLSL_RAYCHIT})
        add_hlsl_subtarget(rchit ${ENTRY})
    endforeach()

    # Add rahit shaders.
    foreach(ENTRY ${ADD_HLSL_RAYAHIT})
        add_hlsl_subtarget(rahit ${ENTRY})
    endforeach()

    # Add rmiss shaders.
    foreach(ENTRY ${ADD_HLSL_RAYMISS})
        add_hlsl_subtarget(rmiss ${ENTRY})
    endforeach()

    # Build shader library target
    set(OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${ADD_HLSL_OUTPUT}")
    get_filename_component(OUTPUT_FILE ${OUTPUT_FILE} ABSOLUTE)
    get_filename_component(OUTPUT_DIRECTORY ${OUTPUT_FILE} DIRECTORY)
    add_custom_command(
        OUTPUT  ${ADD_HLSL_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIRECTORY}
        COMMAND ${CMAKE_HLSL_LINKER} --target-env ${ADD_HLSL_VERSION} ${ADD_HLSL_OBJECTS} -o ${ADD_HLSL_OUTPUT}
        DEPENDS ${ADD_HLSL_OBJECTS})

endfunction()
