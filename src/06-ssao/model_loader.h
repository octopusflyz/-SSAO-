#pragma once

#include <string>

namespace ModelLoader {
    // Simple wrapper to show how to load a glTF using tinygltf; returns true on success
    bool loadGLTF(const std::string &path);
}
