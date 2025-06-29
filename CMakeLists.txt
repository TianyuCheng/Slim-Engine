cmake_minimum_required(VERSION 3.3.0)

#  ____            _           _
# |  _ \ _ __ ___ (_) ___  ___| |_
# | |_) | '__/ _ \| |/ _ \/ __| __|
# |  __/| | | (_) | |  __/ (__| |_
# |_|   |_|  \___// |\___|\___|\__|
#               |__/
# ---------------------------------------------------------------
project(slim LANGUAGES CXX)

#   ____                      _ _
#  / ___|___  _ __ ___  _ __ (_) | ___ _ __
# | |   / _ \| '_ ` _ \| '_ \| | |/ _ \ '__|
# | |__| (_) | | | | | | |_) | | |  __/ |
#  \____\___/|_| |_| |_| .__/|_|_|\___|_|
#                      |_|
# ---------------------------------------------------------------
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(${PROJECT_SOURCE_DIR}/CMake/Compiler.cmake)

#  ____                            _                 _
# |  _ \  ___ _ __   ___ _ __   __| | ___ _ __   ___(_) ___  ___
# | | | |/ _ \ '_ \ / _ \ '_ \ / _` |/ _ \ '_ \ / __| |/ _ \/ __|
# | |_| |  __/ |_) |  __/ | | | (_| |  __/ | | | (__| |  __/\__ \
# |____/ \___| .__/ \___|_| |_|\__,_|\___|_| |_|\___|_|\___||___/
#            |_|
# ---------------------------------------------------------------
add_subdirectory(Dependencies)

#     _                 _
#    / \   ___ ___  ___| |_ ___
#   / _ \ / __/ __|/ _ \ __/ __|
#  / ___ \\__ \__ \  __/ |_\__ \
# /_/   \_\___/___/\___|\__|___/
#
# add assets directory
# ----------------------------------------------------------------
add_definitions(-DSLIM_LIB_ASSETS_DIRECTORY="${PROJECT_SOURCE_DIR}/Library/assets")
add_definitions(-DSLIM_USR_ASSETS_DIRECTORY="${PROJECT_SOURCE_DIR}/Assets")

#  ____  _               _
# / ___|| |__   __ _  __| | ___ _ __
# \___ \| '_ \ / _` |/ _` |/ _ \ '__|
#  ___) | | | | (_| | (_| |  __/ |
# |____/|_| |_|\__,_|\__,_|\___|_|
#
# ---------------------------------------------------------------
include(${PROJECT_SOURCE_DIR}/CMake/GLSL.cmake)
include(${PROJECT_SOURCE_DIR}/CMake/HLSL.cmake)
include(${PROJECT_SOURCE_DIR}/CMake/Project.cmake)

#  _     _ _                    _
# | |   (_) |__  _ __ __ _ _ __(_) ___  ___
# | |   | | '_ \| '__/ _` | '__| |/ _ \/ __|
# | |___| | |_) | | | (_| | |  | |  __/\__ \
# |_____|_|_.__/|_|  \__,_|_|  |_|\___||___/
# ---------------------------------------------------------------
add_subdirectory(Library)

#  _____                           _
# | ____|_  ____ _ _ __ ___  _ __ | | ___  ___
# |  _| \ \/ / _` | '_ ` _ \| '_ \| |/ _ \/ __|
# | |___ >  < (_| | | | | | | |_) | |  __/\__ \
# |_____/_/\_\__,_|_| |_| |_| .__/|_|\___||___/
#                           |_|
# ---------------------------------------------------------------
add_subdirectory(Examples)

#     _                _ _           _   _
#    / \   _ __  _ __ | (_) ___ __ _| |_(_) ___  _ __
#   / _ \ | '_ \| '_ \| | |/ __/ _` | __| |/ _ \| '_ \
#  / ___ \| |_) | |_) | | | (_| (_| | |_| | (_) | | | |
# /_/   \_\ .__/| .__/|_|_|\___\__,_|\__|_|\___/|_| |_|
#         |_|   |_|
# ---------------------------------------------------------------
add_subdirectory(Applications)

#  _____         _
# |_   _|__  ___| |_ ___
#   | |/ _ \/ __| __/ __|
#   | |  __/\__ \ |_\__ \
#   |_|\___||___/\__|___/
# ---------------------------------------------------------------
add_subdirectory(Tests)
