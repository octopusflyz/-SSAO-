#include "model_loader.h"
#include <tiny_gltf.h>
#include <iostream>

bool ModelLoader::loadGLTF(const std::string &path) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    if (!warn.empty()) std::cout << "gltf warn: " << warn << std::endl;
    if (!err.empty()) std::cerr << "gltf err: " << err << std::endl;
    if (!ret) {
        std::cerr << "Failed to load glTF: " << path << std::endl;
        return false;
    }
    std::cout << "Loaded glTF: " << path << " meshes: " << model.meshes.size() << std::endl;
    return true;
}
