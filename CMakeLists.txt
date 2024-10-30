cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(serious LANGUAGES CXX)

option(ENABLE_PROFILER OFF)
set(TRACY_ROOT thirdparty/tracy/public)
if(ENABLE_PROFILER)
    add_compile_definitions(TRACY_ENABLE)
endif()

include(cmake/CopyFile.cmake)
include(cmake/FindVulkan.cmake)
include(cmake/FindSDL3.cmake)

add_subdirectory(thirdparty)

find_program(GLSLC glslc REQUIRED)
execute_process(COMMAND ${GLSLC} ${PROJECT_SOURCE_DIR}/shaders/shader.vert -o ${PROJECT_SOURCE_DIR}/shaders/vert.spv)
execute_process(COMMAND ${GLSLC} ${PROJECT_SOURCE_DIR}/shaders/shader.frag -o ${PROJECT_SOURCE_DIR}/shaders/frag.spv)

file(GLOB_RECURSE SRC src/*.cpp)

add_library(serious STATIC ${SRC})
target_include_directories(serious PUBLIC include)
target_link_libraries(serious PUBLIC Vulkan::Vulkan fmt glm STB tinyobjloader SDL3::SDL3)
target_compile_features(serious PUBLIC cxx_std_17)
target_compile_definitions(serious PUBLIC
    "$<$<CONFIG:DEBUG>:ENABLE_VALIDATION>"
)
target_sources(serious PRIVATE ${TRACY_ROOT}/TracyClient.cpp)
target_include_directories(serious PRIVATE ${TRACY_ROOT}/tracy)

target_compile_options(serious PRIVATE
    -Wall                   # 启用所有常见的警告
    -Wextra                 # 启用额外的警告
    -Wpedantic              # 启用对标准的严格检查
    # -Werror                 # 将所有警告视为错误
    -Wshadow                # 对变量隐藏发出警告
    # -Wconversion            # 类型转换时可能丢失信息的警告
    -Wsign-conversion       # 有符号与无符号转换的警告
    -Wunreachable-code      # 无法到达的代码
    # -Wold-style-cast        # 禁用旧的 C 风格强制转换
    -Woverloaded-virtual    # 虚函数被重载但基类没有虚函数的警告
    -Wnull-dereference      # 对空指针的解引用进行警告
    -Wunused                # 警告未使用的变量
)

add_subdirectory(sandbox)
