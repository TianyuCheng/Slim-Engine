function(add_glsl)

    # Find glslc compiler.
    find_program(CMAKE_GLSL_COMPILER glslc REQUIRED)
    mark_as_advanced(CMAKE_GLSL_COMPILER)

    # Find spirv linker.
    find_program(CMAKE_GLSL_LINKER spirv-link REQUIRED)
    mark_as_advanced(CMAKE_GLSL_LINKER)

    # Parse arguments.
    set(prefix ADD_GLSL)
    set(flags "")
    set(singleValues OUTPUT VERSION COMPUTE VERTEX FRAGMENT GEOMETRY TESSCTRL TESSEVAL RAYGEN RAYCHIT RAYAHIT RAYMISS)
    set(multiValues INCLUDE)
    include (CMakeParseArguments)
    cmake_parse_arguments(${prefix} "${flags}" "${singleValues}" "${multiValues}" ${ARGN})

    # Create common variables
    set(ADD_GLSL_COMPILE_FLAGS -g -O -Os)
    set(ADD_GLSL_OBJECT_DIR "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${ADD_GLSL_OUTPUT}.dir")
    set(ADD_GLSL_OBJECTS "")

    # Prepend -I for each include directory
    set(ADD_GLSL_INCLUDE_DIRS "")
    foreach(INCLUDE ${ADD_GLSL_INCLUDE})
        get_filename_component(ABS_INCLUDE ${INCLUDE} ABSOLUTE)
        list(APPEND ADD_GLSL_INCLUDE_DIRS -I${ABS_INCLUDE})
    endforeach()

    # NOTE: This is a bit crude, because most of the generators does not support DEPFILE.
    # Therefore we scan the whole include directories to scan for any possible dependencies.
    set(ADD_GLSL_INCLUDE_DEPS "")
    foreach(INCLUDE ${ADD_GLSL_INCLUDE})
        get_filename_component(ABS_INCLUDE ${INCLUDE} ABSOLUTE)
        file(GLOB_RECURSE FILES ${ABS_INCLUDE}/*.h)
        list(APPEND ADD_GLSL_INCLUDE_DEPS ${FILES})
    endforeach()

    # Define sub-targets for the final target.
    # Each sub-target is a shader spirv file with entry.
    # All different shader stages use the same compiling logic, so we define a macro.
    macro(add_glsl_subtarget STAGE SOURCE)
        get_filename_component(FILENAME  ${SOURCE} NAME)
        get_filename_component(DIRECTORY ${SOURCE} DIRECTORY)
        set(ADD_GLSL_SUBTARGET "${ADD_GLSL_OBJECT_DIR}/${ENTRY}.spv")
        list(APPEND ADD_GLSL_OBJECTS ${ADD_GLSL_SUBTARGET})
        add_custom_command(
            OUTPUT  ${ADD_GLSL_SUBTARGET}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ADD_GLSL_OBJECT_DIR}
            COMMAND ${CMAKE_GLSL_COMPILER} -MD --target-env=${ADD_GLSL_VERSION}
                                                -fshader-stage=${STAGE}
                                                ${ADD_GLSL_INCLUDE_DIRS}
                                                ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE}
                                                ${ADD_GLSL_COMPILE_FLAGS} -o ${ADD_GLSL_SUBTARGET}
            DEPENDS ${ADD_GLSL_SOURCE} ${ADD_GLSL_INCLUDE_DEPS}
            WORKING_DIRECTORY ${ADD_GLSL_OBJECT_DIR}
            BYPRODUCTS ${ADD_GLSL_OBJECT_DIR}/${ADD_GLSL_SUBTARGET}.d)
        mark_as_advanced(${ADD_GLSL_SUBTARGET})
    endmacro()

    # Add compute shaders.
    foreach(ENTRY ${ADD_GLSL_COMPUTE})
        add_glsl_subtarget(compute ${ENTRY})
    endforeach()

    # Add vertex shaders.
    foreach(ENTRY ${ADD_GLSL_VERTEX})
        add_glsl_subtarget(vertex ${ENTRY})
    endforeach()

    # Add fragment shaders.
    foreach(ENTRY ${ADD_GLSL_FRAGMENT})
        add_glsl_subtarget(fragment ${ENTRY})
    endforeach()

    # Add geometry shaders.
    foreach(ENTRY ${ADD_GLSL_GEOMETRY})
        add_glsl_subtarget(geometry ${ENTRY})
    endforeach()

    # Add tessctrl shaders.
    foreach(ENTRY ${ADD_GLSL_TESSCTRL})
        add_glsl_subtarget(tesscontrol ${ENTRY})
    endforeach()

    # Add tesseval shaders.
    foreach(ENTRY ${ADD_GLSL_TESSEVAL})
        add_glsl_subtarget(tesseval ${ENTRY})
    endforeach()

    # Add rgen shaders.
    foreach(ENTRY ${ADD_GLSL_RAYGEN})
        add_glsl_subtarget(rgen ${ENTRY})
    endforeach()

    # Add rchit shaders.
    foreach(ENTRY ${ADD_GLSL_RAYCHIT})
        add_glsl_subtarget(rchit ${ENTRY})
    endforeach()

    # Add rahit shaders.
    foreach(ENTRY ${ADD_GLSL_RAYAHIT})
        add_glsl_subtarget(rahit ${ENTRY})
    endforeach()

    # Add rmiss shaders.
    foreach(ENTRY ${ADD_GLSL_RAYMISS})
        add_glsl_subtarget(rmiss ${ENTRY})
    endforeach()

    # Build shader library target
    add_custom_command(
        OUTPUT  ${ADD_GLSL_OUTPUT}
        COMMAND ${CMAKE_GLSL_LINKER} --target-env ${ADD_GLSL_VERSION} ${ADD_GLSL_OBJECTS} -o ${ADD_GLSL_OUTPUT}
        DEPENDS ${ADD_GLSL_OBJECTS})
    mark_as_advanced(${ADD_GLSL_OUTPUT})

endfunction()
