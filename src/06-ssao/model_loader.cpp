#include "model_loader.h"
#include <tiny_gltf.h>
#include <iostream>
#include <algorithm>
#include <filesystem>

// Use namespace alias to avoid conflicts
namespace tgltf = tinygltf;

Model::Model() {}
Model::~Model() { cleanup(); }

void Model::cleanup() {
    if (ebo_) { glDeleteBuffers(1, &ebo_); ebo_ = 0; }
    if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
    if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
    if (diffuseTex_) { glDeleteTextures(1, &diffuseTex_); diffuseTex_ = 0; }
    indexCount_ = 0;
    loaded_ = false;
}

bool Model::loadFromFile(const std::string &path) {
    cleanup();

    tgltf::Model model;
    tgltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);

    if (!warn.empty()) std::cout << "glTF warning: " << warn << std::endl;
    if (!err.empty()) {
        std::cerr << "glTF error: " << err << std::endl;
        return false;
    }
    if (!ret) {
        std::cerr << "Failed to load glTF: " << path << std::endl;
        return false;
    }

    if (model.meshes.empty()) {
        std::cerr << "glTF has no meshes" << std::endl;
        return false;
    }

    // Use first mesh / first primitive
    const tinygltf::Mesh &mesh = model.meshes[0];
    if (mesh.primitives.empty()) {
        std::cerr << "Mesh has no primitives" << std::endl;
        return false;
    }
    const tinygltf::Primitive &prim = mesh.primitives[0];

    // Get position data
    auto itPos = prim.attributes.find("POSITION");
    if (itPos == prim.attributes.end()) {
        std::cerr << "No POSITION attribute" << std::endl;
        return false;
    }

    const tgltf::Accessor &posAccessor = model.accessors[itPos->second];
    const tgltf::BufferView &posView = model.bufferViews[posAccessor.bufferView];
    const tgltf::Buffer &posBuffer = model.buffers[posView.buffer];
    const unsigned char *posData = posBuffer.data.data() + posView.byteOffset + posAccessor.byteOffset;

    // Get normal data (optional)
    bool hasNormals = false;
    const unsigned char *normData = nullptr;
    if (auto itN = prim.attributes.find("NORMAL"); itN != prim.attributes.end()) {
        const tgltf::Accessor &nAccessor = model.accessors[itN->second];
        const tgltf::BufferView &nView = model.bufferViews[nAccessor.bufferView];
        const tgltf::Buffer &nBuffer = model.buffers[nView.buffer];
        normData = nBuffer.data.data() + nView.byteOffset + nAccessor.byteOffset;
        hasNormals = true;
    }

    // Get UV data (optional)
    bool hasUV = false;
    const unsigned char *uvData = nullptr;
    if (auto itT = prim.attributes.find("TEXCOORD_0"); itT != prim.attributes.end()) {
        const tgltf::Accessor &tAccessor = model.accessors[itT->second];
        const tgltf::BufferView &tView = model.bufferViews[tAccessor.bufferView];
        const tgltf::Buffer &tBuffer = model.buffers[tView.buffer];
        uvData = tBuffer.data.data() + tView.byteOffset + tAccessor.byteOffset;
        hasUV = true;
    }

    // Get indices
    std::vector<unsigned int> indices;
    if (prim.indices >= 0) {
        const tgltf::Accessor &idxAccessor = model.accessors[prim.indices];
        const tgltf::BufferView &idxView = model.bufferViews[idxAccessor.bufferView];
        const tgltf::Buffer &idxBuffer = model.buffers[idxView.buffer];
        const unsigned char *idxData = idxBuffer.data.data() + idxView.byteOffset + idxAccessor.byteOffset;

        for (size_t i = 0; i < idxAccessor.count; ++i) {
            if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                unsigned short v = *(const unsigned short*)(idxData + i * 2);
                indices.push_back(v);
            } else if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                unsigned int v = *(const unsigned int*)(idxData + i * 4);
                indices.push_back(v);
            } else if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                unsigned char v = *(const unsigned char*)(idxData + i * 1);
                indices.push_back(v);
            }
        }
    }

    // Create vertex data
    struct Vertex { float px,py,pz; float nx,ny,nz; float u,v; };
    std::vector<Vertex> verts(posAccessor.count);

    for (size_t i = 0; i < posAccessor.count; ++i) {
        // Position
        size_t posStride = posView.byteStride ? posView.byteStride : sizeof(float) * 3;
        const float *p = reinterpret_cast<const float*>(posData + i * posStride);
        verts[i].px = p[0]; verts[i].py = p[1]; verts[i].pz = p[2];

        // Normal
        if (hasNormals) {
            const float *n = reinterpret_cast<const float*>(normData + i * sizeof(float) * 3);
            verts[i].nx = n[0]; verts[i].ny = n[1]; verts[i].nz = n[2];
        } else {
            verts[i].nx = verts[i].ny = verts[i].nz = 0.0f;
        }

        // UV
        if (hasUV) {
            const float *t = reinterpret_cast<const float*>(uvData + i * sizeof(float) * 2);
            verts[i].u = t[0]; verts[i].v = t[1];
        } else {
            verts[i].u = verts[i].v = 0.0f;
        }
    }

    // Create OpenGL buffers
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));

    // UV attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));

    glBindVertexArray(0);

    indexCount_ = indices.size();
    loaded_ = true;

    // Load texture
    loadTexture(model, prim, path);

    std::cout << "Successfully loaded model: " << path << std::endl;
    std::cout << "Vertices: " << verts.size() << ", Indices: " << indices.size() << std::endl;
    return true;
}

void Model::loadTexture(const tgltf::Model& model, const tgltf::Primitive& prim, const std::string& modelPath) {
    diffuseTex_ = 0;

    if (prim.material < 0) return;

    const tgltf::Material &mat = model.materials[prim.material];

    // Try to get baseColorTexture
    int texIndex = -1;
    if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
        texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
    }

    if (texIndex < 0 || texIndex >= (int)model.textures.size()) return;

    const tgltf::Texture &texture = model.textures[texIndex];
    if (texture.source < 0 || texture.source >= (int)model.images.size()) return;

    const tgltf::Image &img = model.images[texture.source];

    // If image data is embedded, use it directly
    if (!img.image.empty()) {
        GLenum format = GL_RGBA;
        if (img.component == 3) format = GL_RGB;
        else if (img.component == 1) format = GL_RED;
        else if (img.component == 2) format = GL_RG;

        glGenTextures(1, &diffuseTex_);
        glBindTexture(GL_TEXTURE_2D, diffuseTex_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height, 0, format, GL_UNSIGNED_BYTE, img.image.data());
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        std::cout << "Loaded embedded texture" << std::endl;
        return;
    }

    // If image has URI, try to load external file
    if (!img.uri.empty()) {
        std::cout << "Loading external texture: " << img.uri << std::endl;

        namespace fs = std::filesystem;
        fs::path modelDir = fs::path(modelPath).parent_path();
        std::string filename = fs::path(img.uri).filename().string();

        // Try different paths to find the texture
        std::vector<std::string> texturePaths = {
            (modelDir / img.uri).string(),                    // Relative to model
            (modelDir / "textures" / filename).string(),      // In textures subdir
            (modelDir / ".." / "textures" / filename).string(), // In parent textures
            "assets/revan/textures/" + filename,              // From build dir
            "../assets/revan/textures/" + filename,           // From build/bin
            "D:/geo_lab/final/-SSAO-/assets/revan/textures/" + filename // Absolute
        };

        // Simplified texture loading - for now, skip external textures
        // In a full implementation, you would need to properly handle stb_image
        std::cout << "External texture loading disabled in simplified version: " << img.uri << std::endl;
        std::cout << "Only embedded textures from glTF are supported" << std::endl;
    }
}

void Model::draw() const {
    if (!loaded_) return;
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

bool Model::isLoaded() const {
    return loaded_;
}

GLuint Model::getDiffuseTex() const {
    return diffuseTex_;
}

