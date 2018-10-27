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
#ifndef VOXELSLICER_SHADER_HH
#define VOXELSLICER_SHADER_HH
#include <GL/glew.h>
#include <string>

class shader
{
public:
    shader(const std::string& vsrc, const std::string& fsrc);
    shader(shader&& other);
    ~shader();

    GLint get_uniform(const std::string& name);

    void bind();
private:
    GLuint program;
};

#endif
