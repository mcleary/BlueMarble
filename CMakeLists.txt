
cmake_minimum_required(VERSION 3.12)

project(BlueMarble)

set(CMAKE_CXX_STANDARD 20)

find_package(GLEW REQUIRED)
find_package(glad REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Stb REQUIRED)
find_package(glm REQUIRED)


add_executable(BlueMarble main.cpp
                          Camera.cpp)

target_include_directories(BlueMarble PRIVATE ${Stb_INCLUDE_DIR})
target_link_libraries(BlueMarble PRIVATE glad::glad
                                         glfw)


add_custom_command(TARGET BlueMarble POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${CMAKE_SOURCE_DIR}/shaders" "${CMAKE_BINARY_DIR}/shaders"
                   COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${CMAKE_SOURCE_DIR}/textures" "${CMAKE_BINARY_DIR}/textures")