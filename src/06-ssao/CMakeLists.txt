cmake_minimum_required(VERSION 3.15)

project(ssao_demo)

set(SOURCES
    main.cpp
    shader.cpp
    renderer.cpp
    model_loader.cpp
    fbx_model.cpp
    gbuffer.cpp
)

add_executable(ssao_demo ${SOURCES})

target_include_directories(ssao_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(ssao_demo PRIVATE cxx_std_17)

find_package(OpenGL REQUIRED)

# Link to third-party libraries provided in the workspace
target_link_libraries(ssao_demo PRIVATE glfw glew_s imgui tinygltf assimp::assimp OpenGL::GL glm::glm)

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
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/gbuffer.vert"
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/gbuffer.frag"
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/quad.vert"
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/debug_display.frag"
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/lighting.frag"
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/ssao.frag"
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/ssao_blur.frag"
        "${CMAKE_BINARY_DIR}/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/simple.vert"
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/simple.frag"
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/gbuffer.vert"
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/gbuffer.frag"
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/quad.vert"
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/debug_display.frag"
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/lighting.frag"
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/ssao.frag"
        "${CMAKE_SOURCE_DIR}/src/06-ssao/shaders/ssao_blur.frag"
        "$<TARGET_FILE_DIR:ssao_demo>/shaders/"
)

# Copy assets directory to build directory
add_custom_command(TARGET ssao_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "${CMAKE_BINARY_DIR}/assets"
    COMMENT "Copying assets directory to build directory"
)
