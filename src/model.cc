/*
    Copyright 2018 Julius Ikkala

    This file is part of VoxelSlicer.

    VoxelSlicer is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VoxelSlicer is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VoxelSlicer.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "model.hh"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include "stb_image.h"
#include "shader.hh"

model::model(const std::string& path, GLint interpolation)
{
    Assimp::Importer importer;
    importer.SetPropertyInteger(
        AI_CONFIG_PP_RVC_FLAGS,
        aiComponent_COLORS |
        aiComponent_BONEWEIGHTS |
        aiComponent_ANIMATIONS |
        aiComponent_LIGHTS |
        aiComponent_CAMERAS |
        aiComponent_NORMALS |
        aiComponent_TANGENTS_AND_BITANGENTS
    );

    const aiScene* scene = importer.ReadFile(
        path,
        aiProcessPreset_TargetRealtime_MaxQuality |
        aiProcess_FlipUVs |
        aiProcess_RemoveComponent |
        aiProcess_PreTransformVertices
    );

    if(!scene) throw std::runtime_error("Failed to open file " + path);

    bb_min = glm::vec3(INFINITY);
    bb_max = glm::vec3(-INFINITY);

    for(unsigned i = 0; i < scene->mNumMaterials; ++i)
    {
        aiMaterial* inmat = scene->mMaterials[i];
        materials.push_back(material());
        material& outmat = materials.back();
        outmat.tex = nullptr;
        outmat.color = glm::vec4(0);
        
        aiColor3D color;
        aiString ai_path;
        if(inmat->Get(
            AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0),
            ai_path
        ) == AI_SUCCESS)
        {
            std::string path(ai_path.C_Str());
            if(path != "")
            {
                auto it = textures.find(path);
                if(it == textures.end())
                {
                    // Embedded texture
                    if(path[0] == '*')
                    {
                        unsigned index = strtoul(path.c_str()+1, nullptr, 10);
                        aiTexture* tex = scene->mTextures[index];
                        it = textures.emplace(
                            path,
                            texture(
                                GL_BGRA,
                                tex->pcData,
                                glm::uvec2(tex->mWidth, tex->mHeight),
                                interpolation
                            )
                        ).first;
                    }
                    // External texture
                    else it = textures.emplace(
                        path,
                        texture(path, interpolation)
                    ).first;
                }
                outmat.tex = &it->second;
            }
            else
            {
                std::cerr << "Warning: asset importer gave empty path, a "
                    "texture will be missing." << std::endl;
            }
        }
        if(inmat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
        {
            outmat.color.r = color.r;
            outmat.color.g = color.g;
            outmat.color.b = color.b;
            outmat.color.a = 1.0f;
        }
    }

    for(unsigned i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh* inmesh = scene->mMeshes[i];
        if(!inmesh->HasFaces()) throw std::runtime_error("Mesh has no faces");
        if(!inmesh->HasPositions())
            throw std::runtime_error("Mesh has no positions");
        meshes.push_back(mesh(inmesh, bb_min, bb_max));
    }
}

model::~model()
{
}

void model::draw(glm::mat4 proj, shader& textured, shader& no_texture)
{
    for(mesh& m: meshes)
    {
        material* mat = &materials[m.material_index];
        GLint mvp;
        if(mat->tex)
        {
            textured.bind();
            GLint albedo_tex = textured.get_uniform("albedo_tex");
            mvp = textured.get_uniform("mvp");
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mat->tex->tex);
            glUniform1i(albedo_tex, 0);
        }
        else
        {
            no_texture.bind();
            GLint albedo = no_texture.get_uniform("albedo");
            mvp = no_texture.get_uniform("mvp");
            glUniform4fv(albedo, 1, (float*)&mat->color);
        }

        glUniformMatrix4fv(mvp, 1, false, (float*)&proj);
        glBindVertexArray(m.vao);
        glDrawElements(GL_TRIANGLES, m.index_count, GL_UNSIGNED_INT, 0);
    }
}

void model::init_gl()
{
    for(auto& pair: textures) pair.second.init_gl();
    for(auto& m: meshes) m.init_gl();
}

void model::get_bb(glm::vec3& bb_min, glm::vec3& bb_max) const
{
    bb_min = this->bb_min;
    bb_max = this->bb_max;
}

model::texture::texture(const std::string& path, GLint interpolation)
: data(0), size(0), tex(0), format(GL_RGBA), interpolation(interpolation)
{
    int n;
    data = stbi_load(path.c_str(), (int*)&size.x, (int*)&size.y, &n, 4);
    if(!data) throw std::runtime_error("Failed to read texture " + path);
}

model::texture::texture(
    GLenum format, void* data, glm::uvec2 size, GLint interpolation
)
: data(data), size(size), tex(0), format(format), interpolation(interpolation)
{}

model::texture::texture(texture&& other)
:   data(other.data), size(other.size), tex(other.tex), format(other.format),
    interpolation(other.interpolation)
{
    other.data = nullptr;
    other.tex = 0;
}

model::texture::~texture()
{
    if(data) free(data);
    if(tex) glDeleteTextures(1, &tex);
}

void model::texture::init_gl()
{
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y,
        0, format, GL_UNSIGNED_BYTE, data
    );
    if(interpolation == GL_LINEAR_MIPMAP_LINEAR)
        glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolation);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolation);
}

model::mesh::mesh(aiMesh* inmesh, glm::vec3& bb_min, glm::vec3& bb_max)
: vao(0), ibo(0), vbo(0)
{
    indices = new uint32_t[inmesh->mNumFaces*3];
    index_count = inmesh->mNumFaces*3;
    for(unsigned i = 0; i < inmesh->mNumFaces; ++i)
    {
        aiFace* face = inmesh->mFaces + i;
        indices[i*3] = face->mIndices[0];
        indices[i*3+1] = face->mIndices[1];
        indices[i*3+2] = face->mIndices[2];
    }
    
    has_uv = inmesh->GetNumUVChannels() >= 1;
    unsigned stride = has_uv ? 5 : 3;

    vertex_count = inmesh->mNumVertices;
    vertices = new float[stride*vertex_count];

    for(unsigned i = 0; i < vertex_count; ++i)
    {
        aiVector3D ai_p = inmesh->mVertices[i];
        glm::vec3 p(ai_p.x, ai_p.y, ai_p.z);
        bb_min = glm::min(p, bb_min);
        bb_max = glm::max(p, bb_max);
        vertices[i*stride] = p.x;
        vertices[i*stride+1] = p.y;
        vertices[i*stride+2] = p.z;
        if(has_uv)
        {
            vertices[i*stride+3] = inmesh->mTextureCoords[0][i].x;
            vertices[i*stride+4] = inmesh->mTextureCoords[0][i].y;
        }
    }

    material_index = inmesh->mMaterialIndex;
}

model::mesh::mesh(mesh&& other)
:   material_index(other.material_index),
    indices(other.indices),
    index_count(other.index_count),
    vertices(other.vertices),
    vertex_count(other.vertex_count),
    has_uv(other.has_uv),
    vao(other.vao),
    ibo(other.ibo),
    vbo(other.vbo)
{
    other.indices = nullptr;
    other.vertices = nullptr;
    other.vao = 0;
    other.ibo = 0;
    other.vbo = 0;
}

model::mesh::~mesh()
{
    if(indices) delete [] indices;
    if(vertices) delete [] vertices;
    if(vao) glDeleteBuffers(1, &vao);
    if(ibo) glDeleteBuffers(1, &ibo);
    if(vbo) glDeleteBuffers(1, &vbo);
}

void model::mesh::init_gl()
{
    unsigned stride = has_uv ? 5 : 3;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(float)*stride*vertex_count,
        vertices,
        GL_STATIC_DRAW
    );

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(uint32_t)*index_count,
        indices,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        stride*sizeof(float),
        nullptr
    );
    glEnableVertexAttribArray(0);

    if(has_uv)
    {
        glVertexAttribPointer(
            1,
            2,
            GL_FLOAT,
            GL_FALSE,
            stride*sizeof(float),
            (void*)(3*sizeof(float))
        );
        glEnableVertexAttribArray(1);
    }
}
