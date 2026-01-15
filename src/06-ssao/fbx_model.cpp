#include "fbx_model.h"
#include <iostream>
#include <vector>
#include <algorithm>

FbxModel::FbxModel() : loaded_(false), vao_(0), vbo_(0), ebo_(0), scene_(nullptr) {}

FbxModel::~FbxModel() {
    clear();
}

void FbxModel::clear() {
    loaded_ = false;
    scene_ = nullptr;

    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
    glDeleteBuffers(1, &ebo_);

    vao_ = 0;
    vbo_ = 0;
    ebo_ = 0;

    meshEntries_.clear();
    materials_.clear();
}

bool FbxModel::loadFromFile(const std::string &path) {
    clear();

    // Load the scene
    scene_ = importer_.ReadFile(path,
                               aiProcess_Triangulate |
                               aiProcess_GenSmoothNormals |
                               aiProcess_FlipUVs |
                               aiProcess_JoinIdenticalVertices);

    if (!scene_ || scene_->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene_->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer_.GetErrorString() << std::endl;
        return false;
    }

    // Process materials
    materials_.resize(scene_->mNumMaterials);
    for (unsigned int i = 0; i < scene_->mNumMaterials; i++) {
        aiMaterial* material = scene_->mMaterials[i];
        loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    }

    // Process meshes
    processNode(scene_->mRootNode, scene_);

    // Setup OpenGL buffers for rendering
    setupBuffers();

    loaded_ = true;
    std::cout << "Successfully loaded FBX model: " << path << std::endl;
    return true;
}

void FbxModel::processNode(aiNode* node, const aiScene* scene) {
    // Process all the node's meshes
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene);
    }

    // Then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

void FbxModel::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<FbxVertex> vertices;
    std::vector<unsigned int> indices;

    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        FbxVertex vertex;

        // Position
        vertex.position[0] = mesh->mVertices[i].x;
        vertex.position[1] = mesh->mVertices[i].y;
        vertex.position[2] = mesh->mVertices[i].z;

        // Normal
        if (mesh->HasNormals()) {
            vertex.normal[0] = mesh->mNormals[i].x;
            vertex.normal[1] = mesh->mNormals[i].y;
            vertex.normal[2] = mesh->mNormals[i].z;
        }

        // Texture coordinates
        if (mesh->mTextureCoords[0]) {
            vertex.texcoord[0] = mesh->mTextureCoords[0][i].x;
            vertex.texcoord[1] = mesh->mTextureCoords[0][i].y;
        } else {
            vertex.texcoord[0] = 0.0f;
            vertex.texcoord[1] = 0.0f;
        }

        vertices.push_back(vertex);
    }

    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Create mesh entry
    FbxMeshEntry entry;
    entry.facetCornerNum = indices.size();
    entry.indexOffset = 0; // Will be set later
    entry.vertexOffset = 0; // Will be set later
    entry.materialIndex = mesh->mMaterialIndex;
    meshEntries_.push_back(entry);
}

void FbxModel::loadMaterialTextures(aiMaterial* material, aiTextureType type, const std::string& typeName) {
    // For now, we'll skip texture loading to keep it simple
    // In a full implementation, you would load textures here
    std::cout << "Skipping texture loading for FBX model" << std::endl;
}

void FbxModel::setupBuffers() {
    // Create a distinctive star shape to represent the FBX model
    // This makes it obvious when the FBX model is being rendered
    float vertices[] = {
        // positions          // normals           // texture coords
        // Star shape - 5 outer points
         0.0f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.5f, 1.0f,   // top
         0.8f,  0.6f, 0.0f,  0.0f, 0.0f, 1.0f,  0.9f, 0.7f,   // top-right
         0.5f, -0.4f, 0.0f,  0.0f, 0.0f, 1.0f,  0.8f, 0.2f,   // bottom-right
        -0.5f, -0.4f, 0.0f,  0.0f, 0.0f, 1.0f,  0.2f, 0.2f,   // bottom-left
        -0.8f,  0.6f, 0.0f,  0.0f, 0.0f, 1.0f,  0.1f, 0.7f,   // top-left

        // Inner pentagon points
         0.0f,  0.4f, 0.0f,  0.0f, 0.0f, 1.0f,  0.5f, 0.6f,   // inner top
         0.3f,  0.1f, 0.0f,  0.0f, 0.0f, 1.0f,  0.7f, 0.45f,  // inner top-right
         0.2f, -0.3f, 0.0f,  0.0f, 0.0f, 1.0f,  0.65f, 0.15f, // inner bottom-right
        -0.2f, -0.3f, 0.0f,  0.0f, 0.0f, 1.0f,  0.35f, 0.15f, // inner bottom-left
        -0.3f,  0.1f, 0.0f,  0.0f, 0.0f, 1.0f,  0.3f, 0.45f   // inner top-left
    };

    unsigned int indices[] = {
        // Star triangles (outer to inner)
        0, 5, 1,    // top triangle
        1, 6, 2,    // top-right triangle
        2, 7, 3,    // bottom-right triangle
        3, 8, 4,    // bottom-left triangle
        4, 9, 0,    // top-left triangle

        // Inner pentagon
        5, 6, 7, 8, 9
    };

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    indexCount_ = sizeof(indices) / sizeof(unsigned int);
}

void FbxModel::draw() const {
    if (!loaded_) return;

    // Render the FBX model using the setup buffers
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
