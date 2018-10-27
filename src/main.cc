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
#include <EGL/egl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <memory>
#include <getopt.h>
#include "stb_image_write.h"
#include "shader.hh"
#include "model.hh"
#define GL_MAJOR 3
#define GL_MINOR 3
#define HELP 1
#define DIMENSIONS 'd'
#define OUTPUT 'o'

struct
{
    EGLDisplay display;
    EGLint major, minor;
    EGLConfig cfg;
    EGLSurface surface;
    EGLContext ctx;
    glm::uvec2 size;
} egl_data;

struct
{
    std::string input_path;
    std::string output_path = "slice";
    glm::ivec3 dim = glm::ivec3(-1);
} options;

const std::string fshader_textured = 
    "#version 330 core\n"
    "in vec2 uv;\n"
    "uniform sampler2D albedo_tex;\n"
    "out vec4 color;\n"
    "void main() {\n"
    "    color = texture(albedo_tex, uv);\n"
    "}";

const std::string vshader_textured = 
    "#version 330 core\n"
    "layout (location = 0) in vec3 pos;\n"
    "layout (location = 1) in vec2 in_uv;\n"
    "uniform mat4 mvp;\n"
    "out vec2 uv;\n"
    "void main() {\n"
    "    gl_Position = mvp * vec4(pos, 1.0f);\n"
    "    uv = in_uv;\n"
    "}";

const std::string fshader_no_texture = 
    "#version 330 core\n"
    "uniform vec4 albedo;\n"
    "out vec4 color;\n"
    "void main() { color = albedo; }";

const std::string vshader_no_texture = 
    "#version 330 core\n"
    "layout (location = 0) in vec3 pos;\n"
    "uniform mat4 mvp;\n"
    "void main() {\n"
    "    gl_Position = mvp * vec4(pos, 1.0f);\n"
    "}";

static bool parse_args(int argc, char** argv)
{
    int indexptr = 0;

    struct option longopts[] = {
        { "help", no_argument, NULL, HELP },
        { "dimensions", required_argument, NULL, DIMENSIONS },
        { "output", required_argument, NULL, OUTPUT }
    };

    int val = 0;
    while((val = getopt_long(argc, argv, "d:o:", longopts, &indexptr)) != -1)
    {
        char* endptr = optarg-1;
        unsigned axis = 0;
        switch(val)
        {
        case DIMENSIONS:
            for(axis = 0; axis < 3; ++axis)
            {
                options.dim[axis] = strtoul(endptr+1, &endptr, 10);
                if(*endptr != 'x') break;
            }
            if(*endptr != 0)
            {
                printf("Invalid dimensions format!\n");
                goto help_print;
            }

            if(axis == 0)
            {
                options.dim.z = options.dim.x;
                options.dim.x = -1;
            }
            else if(axis == 1)
            {
                options.dim.z = options.dim.y;
                options.dim.y = -1;
            }
            break;
        case OUTPUT:
            options.output_path = optarg;
            break;
        case HELP:
            goto help_print;
        default: break;
        }
    }

    if(optind + 1 != argc)
    {
        goto help_print;
    }
    options.input_path = argv[optind];

    return true;

help_print:
    printf(
        "Usage: %s [-d dimensions] [-o output_prefix] model_file\n"
        "\ndimensions defines the size of the output. It can have one of the "
        "following formats:\n"
        "\tWIDTHxHEIGHTxLAYERS\n"
        "\tWIDTHxLAYERS\n"
        "\tLAYERS\n"
        "WIDTH and HEIGHT define the size of the output image, LAYERS defines "
        "the number\nof output images. If either HEIGHT or WIDTH are missing, "
        "they are calculated\nbased on model dimensions such that the aspect "
        "ratio is correct.\n"
        "\noutput_prefix is the prefix for all output images.\n",
        argv[0]
    );
    return false;
}

static glm::uvec2 except(glm::uvec3 dim, unsigned index)
{
    switch(index)
    {
    case 0: return glm::uvec2(dim.y, dim.z);
    case 1: return glm::uvec2(dim.x, dim.z);
    case 2: return glm::uvec2(dim.x, dim.y);
    default: return glm::uvec2(0);
    }
}

static glm::uvec2 max2(glm::uvec3 dim)
{
    if(dim.x < dim.y && dim.x < dim.z) return glm::uvec2(dim.y, dim.z);
    else if(dim.y < dim.x && dim.y < dim.z) return glm::uvec2(dim.x, dim.z);
    else return glm::uvec2(dim.x, dim.y);
}

static int num_len(uint64_t u, int base)
{
    size_t len = 1;
    while((u /= base)) len++;
    return len;
}

static bool init(glm::uvec3 dim)
{
    // Make sure all directions can be rendered to the same buffer
    egl_data.size = max2(dim);

    EGLint cfg_attribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_STENCIL_SIZE, 8,
        EGL_SAMPLES, 0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_NONE
    };

    EGLint surface_attribs[] = {
        EGL_WIDTH, (EGLint)egl_data.size.x,
        EGL_HEIGHT, (EGLint)egl_data.size.y,
        EGL_NONE
    };

    EGLint ctx_attribs[] = {
        EGL_CONTEXT_MAJOR_VERSION, GL_MAJOR,
        EGL_CONTEXT_MINOR_VERSION, GL_MINOR,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        EGL_NONE
    };
    GLenum err;

    egl_data.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    if(
        egl_data.display == EGL_NO_DISPLAY ||
        !eglInitialize(egl_data.display, &egl_data.major, &egl_data.minor)
    ){
        std::cerr << "Unable to initialize EGL\n";
        return false;
    }

    EGLint config_count;

    if(
        !eglChooseConfig(
            egl_data.display, cfg_attribs, &egl_data.cfg, 1, &config_count
        ) ||
        config_count == 0
    ){
        std::cerr << "Unable to find compatible EGL config\n";
        goto fail;
    }

    egl_data.surface = eglCreatePbufferSurface(
        egl_data.display, egl_data.cfg, surface_attribs
    );
    if(egl_data.surface == EGL_NO_SURFACE)
    {
        std::cerr << "Unable to create surface\n";
        goto fail;
    }
    eglBindAPI(EGL_OPENGL_API);

    egl_data.ctx = eglCreateContext(
        egl_data.display, egl_data.cfg, EGL_NO_CONTEXT, ctx_attribs
    );
    if(egl_data.ctx == EGL_NO_CONTEXT)
    {
        std::cerr << "Failed to create context for OpenGL "
            << GL_MAJOR << "." << GL_MINOR
            << std::endl;
        goto fail;
    }

    eglMakeCurrent(
        egl_data.display, egl_data.surface, egl_data.surface, egl_data.ctx
    );

    glewExperimental = GL_TRUE;
    err = glewInit();
    if(err != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW\n";
        goto fail;
    }

    return true;
fail:
    eglTerminate(egl_data.display);
    return false;
}

static void deinit()
{
    eglTerminate(egl_data.display);
}

struct voxel
{
    glm::vec4 color = glm::vec4(0);
    unsigned count = 0;
};

class volume
{
public:
    volume(glm::uvec3 dim)
    :   voxels(new voxel[dim.x*dim.y*dim.z]),
        dim(dim)
    {
        glm::uvec2 sz = max2(dim);
        layer_buffer = new uint8_t[sz.x*sz.y*4];
        stencil_buffer = new uint8_t[sz.x*sz.y];
    }

    ~volume()
    {
        delete [] voxels;
        delete [] layer_buffer;
        delete [] stencil_buffer;
    }

    voxel& operator[](glm::uvec3 pos) const
    {
        voxel* v = voxels + pos.z*dim.x*dim.y + pos.y*dim.x + pos.x;
        return *v;
    }

    void write_voxel(glm::uvec3 pos, glm::vec4 color)
    {
        voxel& v = operator[](pos);
        v.color = ((float)v.count * v.color + color) / (v.count + 1.0f);
        v.count++;
    }

    glm::uvec3 get_dim() const { return dim; }

    glm::uvec2 get_size(unsigned axis) const
    {
        return except(dim, axis);
    }

    glm::uvec3 get_layer_pos(
        unsigned layer_index, unsigned axis, glm::uvec2 p
    ) const
    {
        switch(axis)
        {
        case 0: return glm::uvec3(layer_index, p.x, p.y);
        case 1: return glm::uvec3(p.x, layer_index, p.y);
        case 2: return glm::uvec3(p.x, p.y, layer_index);
        default: return glm::uvec3(0);
        }
    }

    void read_gl_layer(unsigned layer_index, unsigned axis)
    {
        glm::uvec2 size = get_size(axis);
        glReadPixels(
            0, 0,
            size.x, size.y,
            GL_RGBA, GL_UNSIGNED_BYTE, layer_buffer
        );
        glReadPixels(
            0, 0,
            size.x, size.y,
            GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencil_buffer
        );

        for(unsigned y = 0; y < size.y; ++y)
        {
            for(unsigned x = 0; x < size.x; ++x)
            {
                unsigned o = x + y * size.x;
                if(stencil_buffer[o] == 0) continue;

                glm::vec4 color = {
                    layer_buffer[o*4], layer_buffer[o*4+1],
                    layer_buffer[o*4+2], layer_buffer[o*4+3]
                };
                color /= 255;
                glm::uvec3 pos = get_layer_pos(
                    layer_index, axis, glm::uvec2(x, y)
                );
                write_voxel(pos, color);
            }
        }
    }

    void write_layers(const std::string& path_prefix, unsigned axis)
    {
        glm::uvec2 size = get_size(axis);
        unsigned layer_str_width = num_len(dim[axis], 10);
        for(unsigned layer = 0; layer < dim[axis]; ++layer)
        {
            std::stringstream path;
            path << path_prefix
                 << std::setw(layer_str_width) << std::setfill('0') << layer
                 << ".png";
            for(unsigned y = 0; y < size.y; ++y)
            {
                for(unsigned x = 0; x < size.x; ++x)
                {
                    glm::vec4 color = operator[](get_layer_pos(
                        layer, axis, glm::uvec2(x, y)
                    )).color;
                    unsigned o = x + y * size.x;
                    layer_buffer[o*4] = color.x*255;
                    layer_buffer[o*4+1] = color.y*255;
                    layer_buffer[o*4+2] = color.z*255;
                    layer_buffer[o*4+3] = color.w*255;
                }
            }
            stbi_write_png(
                path.str().c_str(), size.x, size.y, 4, layer_buffer, 4*size.x
            );
        }
    }

private:
    struct voxel* voxels;
    glm::uvec3 dim;
    uint8_t* layer_buffer;
    uint8_t* stencil_buffer;
};

static glm::uvec3 deduce_dim(glm::ivec3 arg, model& m)
{
    glm::vec3 res(arg);
    glm::vec3 bb_min, bb_max;
    m.get_bb(bb_min, bb_max);
    glm::vec3 size = bb_max - bb_min;

    if(arg.z <= 0) res.z = 10;
    if(arg.x <= 0) res.x = res.z / size.z * size.x;
    if(arg.y <= 0) res.y = res.x / size.x * size.y;
    return glm::uvec3(glm::round(res));
}

static glm::mat4 get_proj(
    glm::uvec3 dim,
    unsigned axis,
    unsigned layer,
    model& m
){
    glm::vec3 bb_min, bb_max;
    m.get_bb(bb_min, bb_max);
    glm::vec3 size = bb_max - bb_min;
    float depth = size[axis];
    float step = depth/dim[axis];
    float far = bb_min[axis] + step * layer;
    float near = bb_min[axis] + step * (layer + 1);
    glm::vec2 left_bottom;
    glm::vec2 right_top;

    glm::mat4 base;
    switch(axis)
    {
    case 0:
        base = glm::mat4(
            0,1,0,0,
            0,0,1,0,
            1,0,0,0,
            0,0,0,1
        );
        left_bottom = glm::vec2(bb_min.y, bb_min.z);
        right_top = glm::vec2(bb_max.y, bb_max.z);
        break;
    case 1:
        base = glm::mat4(
            1,0,0,0,
            0,0,-1,0,
            0,1,0,0,
            0,0,0,1
        );
        left_bottom = glm::vec2(bb_min.x, bb_min.z);
        right_top = glm::vec2(bb_max.x, bb_max.z);
        break;
    default:
    case 2:
        base = glm::mat4(1.0f);
        left_bottom = glm::vec2(bb_min.x, bb_min.y);
        right_top = glm::vec2(bb_max.x, bb_max.y);
        break;
    }
    glm::mat4 proj = glm::ortho(
        left_bottom.x,
        right_top.x,
        left_bottom.y,
        right_top.y,
        near,
        far
    );
    return proj * base;
}

int main(int argc, char** argv)
{
    if(!parse_args(argc, argv)) return 1;

    std::unique_ptr<model> m;
    try
    {
        m.reset(new model(options.input_path));
    }
    catch(const std::runtime_error& err)
    {
        std::cerr << err.what() << std::endl;
        return 2;
    }

    glm::uvec3 dim = deduce_dim(options.dim, *m);

    // _NOT_ sparse, so large sizes will kill your performance and memory
    volume v(dim);
    if(!init(dim))
        return 3;

    {
        shader no_texture(vshader_no_texture, fshader_no_texture);
        shader textured(vshader_textured, fshader_textured);

        // Both front and back faces in one pass to avoid extra passes
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glStencilMask(0xFF);

        // Disable padding in glReadPixels to make reading simpler
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearStencil(0);

        m->init_gl();

        // Render scene from all axes
        for(unsigned axis = 0; axis < 3; ++axis)
        {
            glm::uvec2 size = v.get_size(axis);
            glViewport(0, 0, size.x, size.y);
            // Render all layers
            for(unsigned layer = 0; layer < dim[axis]; ++layer)
            {
                glClear(
                    GL_COLOR_BUFFER_BIT |
                    GL_STENCIL_BUFFER_BIT |
                    GL_DEPTH_BUFFER_BIT
                );
                glm::mat4 proj(get_proj(dim, axis, layer, *m));
                m->draw(proj, textured, no_texture);
                v.read_gl_layer(layer, axis);
            }
        }

        v.write_layers(options.output_path, 2);
    }
    deinit();
    return 0;
}
