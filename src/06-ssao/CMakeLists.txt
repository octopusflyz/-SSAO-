cmake_minimum_required(VERSION 3.15)

project(ssao_demo)

set(SOURCES
    main.cpp
    shader.cpp
    renderer.cpp
    model_loader.cpp
)

add_executable(ssao_demo ${SOURCES})

target_include_directories(ssao_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(ssao_demo PRIVATE cxx_std_17)

find_package(OpenGL REQUIRED)

# Link to third-party libraries provided in the workspace
target_link_libraries(ssao_demo PRIVATE glfw glew_s imgui tinygltf OpenGL::GL glm::glm)

# Ensure the GLEW headers are visible to this target (compiler couldn't find <GL/glew.h>)
target_include_directories(ssao_demo PRIVATE ${CMAKE_SOURCE_DIR}/third_party/glew/include)
# Ensure ImGui headers are visible to this target (compiler couldn't find "imgui.h")
target_include_directories(ssao_demo PRIVATE ${CMAKE_SOURCE_DIR}/third_party/imgui)

# Copy shaders to build directory and the executable's runtime directory for runtime loading
add_custom_command(TARGET ssao_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/shaders"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:ssao_demo>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/simple.vert"
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/simple.frag"
        "${CMAKE_BINARY_DIR}/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/simple.vert"
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/simple.frag"
        "$<TARGET_FILE_DIR:ssao_demo>/shaders/"
)

# Provide shader source dir at compile time for runtime fallback lookup
target_compile_definitions(ssao_demo PRIVATE SHADER_SOURCE_DIR=\"${CMAKE_SOURCE_DIR}/src/06-ssao/shaders\")
