cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(serious LANGUAGES CXX)

include(cmake/CopyFile.cmake)
include(cmake/FindVulkan.cmake)
include(cmake/FindSDL3.cmake)

add_subdirectory(thirdparty)

find_program(GLSLC glslc REQUIRED)
execute_process(COMMAND ${GLSLC} ${PROJECT_SOURCE_DIR}/shaders/shader.vert -o ${PROJECT_SOURCE_DIR}/shaders/vert.spv)
execute_process(COMMAND ${GLSLC} ${PROJECT_SOURCE_DIR}/shaders/shader.frag -o ${PROJECT_SOURCE_DIR}/shaders/frag.spv)

aux_source_directory(src/ SRC)

add_library(serious STATIC ${SRC})
target_include_directories(serious PUBLIC .)
target_link_libraries(serious PUBLIC Vulkan::Vulkan fmt)
target_compile_features(serious PUBLIC cxx_std_17)
target_compile_definitions(serious PUBLIC "$<$<CONFIG:DEBUG>:ENABLE_VALIDATION>")

add_subdirectory(sandbox)
