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
#ifndef VOXELSLICER_MODEL_HH
#define VOXELSLICER_MODEL_HH
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <map>

class aiMesh;
class shader;
class model
{
public:
    model(const std::string& path, GLint texture_interpolation);
    ~model();

    void init_gl();
    void draw(glm::mat4 proj, shader& textured, shader& no_texture);

    void get_bb(glm::vec3& bb_min, glm::vec3& bb_max) const;

private:
    struct texture
    {
        texture(const std::string& path, GLint interpolation);
        texture(
            GLenum format, void* data, glm::uvec2 size, GLint interpolation
        );
        texture(texture&& other);
        ~texture();

        void init_gl();

        void* data;
        glm::uvec2 size;
        GLuint tex;
        GLenum format;
        GLint interpolation;
    };

    struct material
    {
        texture* tex;
        glm::vec4 color;
    };

    struct mesh
    {
        mesh(aiMesh* inmesh, glm::vec3& bb_min, glm::vec3& bb_max);
        mesh(mesh&& other);
        ~mesh();

        void init_gl();

        unsigned material_index;

        uint32_t* indices;
        unsigned index_count;

        float* vertices;
        unsigned vertex_count;
        bool has_uv;

        GLuint vao, ibo, vbo;
    };

    std::map<std::string, texture> textures;
    std::vector<material> materials;
    std::vector<mesh> meshes;

    glm::vec3 bb_min, bb_max;
};

#endif
