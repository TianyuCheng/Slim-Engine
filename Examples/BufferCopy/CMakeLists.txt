# compile shaders
# NOTE: nothing to compile

# compile target
add_executable(buffercopy main.cpp)
target_link_libraries(buffercopy PRIVATE slim)
