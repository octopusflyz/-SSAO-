SSAO Demo (skeleton)

Build & Run (Windows, from repository root):

1) Create a build directory and run CMake
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release

2) Place assets (models/hdr/textures) under the top-level `assets/` folder, e.g. `assets/scene.glb`.

3) Run the executable `ssao_demo` (or the appropriate .exe in build output). The demo is a minimal skeleton: window + ImGui + simple shader and a model loader stub (tinygltf). Use it as a starting point for adding G-buffer/SSAO passes.

Notes:
- Shaders are copied to the build folder at post-build; you can edit them in `src/06-ssao/shaders/`.
- This skeleton links to `glfw`, `glew_s`, `imgui`, `tinygltf`, and `glm` already included in `third_party/`.
