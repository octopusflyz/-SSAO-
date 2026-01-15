#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <GL/glew.h>

struct FbxVertex {
    float position[3];
    float texcoord[2];
    float normal[3];

    FbxVertex() { memset(this, 0, sizeof(FbxVertex)); }
    FbxVertex(aiVector3D _p, aiVector2D _tc, aiVector3D _n) {
        memcpy(position, &_p, sizeof(position));
        memcpy(texcoord, &_tc, sizeof(texcoord));
        memcpy(normal, &_n, sizeof(normal));
    }
};

struct FbxMeshEntry {
    unsigned int facetCornerNum;
    unsigned int indexOffset;
    unsigned int vertexOffset;
    unsigned int materialIndex;
};

struct FbxMaterial {
    GLuint diffuseTexture;

    FbxMaterial() : diffuseTexture(0) {}
};

class FbxModel {
public:
    FbxModel();
    ~FbxModel();

    bool loadFromFile(const std::string &path);
    void draw() const;
    bool isLoaded() const { return loaded_; }

private:
    bool loaded_;
    GLuint vao_;
    GLuint vbo_;
    GLuint ebo_;
    int indexCount_;

    std::vector<FbxMeshEntry> meshEntries_;
    std::vector<FbxMaterial> materials_;

    Assimp::Importer importer_;
    const aiScene* scene_;

    void clear();
    void processNode(aiNode* node, const aiScene* scene);
    void processMesh(aiMesh* mesh, const aiScene* scene);
    void loadMaterialTextures(aiMaterial* material, aiTextureType type, const std::string& typeName);
    void setupBuffers();
};
