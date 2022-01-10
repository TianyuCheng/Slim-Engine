# compile shaders
# NOTE: nothing to compile

# compile target
add_executable(dearimgui main.cpp)
target_link_libraries(dearimgui PRIVATE slim)
