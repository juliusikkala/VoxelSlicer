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
#include "shader.hh"
#include <stdexcept>
#include <iostream>

GLuint compile_shader(GLenum type, const std::string& src)
{
    GLuint shader = glCreateShader(type);
    const char* c_src = src.c_str();
    glShaderSource(shader, 1, &c_src, NULL);
    glCompileShader(shader);

    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if(status != GL_TRUE)
    {
        GLsizei length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        char* err = new char[length+1];
        glGetShaderInfoLog(shader, length+1, &length, err);
        std::string err_str(err);
        delete [] err;
        throw std::runtime_error(err_str);
    }
    return shader;
}

shader::shader(const std::string& vsrc, const std::string& fsrc)
{
    GLuint vshader = compile_shader(GL_VERTEX_SHADER, vsrc);
    GLuint fshader = compile_shader(GL_FRAGMENT_SHADER, fsrc);
    program = glCreateProgram();
    glAttachShader(program, vshader);
    glAttachShader(program, fshader);
    glLinkProgram(program);

    GLint status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if(status != GL_TRUE)
    {
        GLsizei length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        char* err = new char[length+1];
        glGetProgramInfoLog(program, length+1, &length, err);
        std::string err_str(err);
        delete [] err;
        throw std::runtime_error(err_str);
    }

    glDeleteShader(vshader);
    glDeleteShader(fshader);
}

shader::shader(shader&& other)
: program(other.program)
{
    other.program = 0;
}

shader::~shader()
{
    if(program) glDeleteProgram(program);
}

GLint shader::get_uniform(const std::string& name)
{
    return glGetUniformLocation(program, name.c_str());
}

void shader::bind()
{
    glUseProgram(program);
}
