#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "internal_data.hpp"
#include "util.hpp"
#include "window.hpp"
#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

#define CASE(a, b) case paz::PrimitiveType::a: return GL_##b;
#define CHECK_UNIFORM if(!_data->_shader->_uniformIds.count(name)) return;

static constexpr float Transparent[] = {0.f, 0.f, 0.f, 0.f};
static int NextSlot = 0;

static bool DepthTestEnabled = false;
static bool DepthMaskEnabled = true;
static bool BlendEnabled = false;

static GLenum primitive_type(paz::PrimitiveType t)
{
    switch(t)
    {
        CASE(Points, POINTS)
        CASE(Lines, LINES)
        CASE(LineStrip, LINE_STRIP)
        CASE(Triangles, TRIANGLES)
        CASE(TriangleStrip, TRIANGLE_STRIP);
        default: throw std::runtime_error("Invalid primitive type.");
    }
}

static void check_attributes(const std::vector<unsigned int>& a, const std::
    unordered_map<unsigned int, unsigned int>& b)
{
    if(a.size() < b.size())
    {
        throw std::invalid_argument("Vertex buffer has too few of attributes fo"
            "r shader (got " + std::to_string(a.size()) + ", expected at least "
            + std::to_string(b.size()) + ").");
    }
    for(std::size_t i = 0; i < a.size(); ++i)
    {
        if(b.count(i) && a[i] != b.at(i))
        {
            throw std::invalid_argument("Vertex buffer attribute " + std::
                to_string(i) + " type mismatch (got " + std::to_string(a[i]) +
                ", expected " + std::to_string(b.at(i)) + ").");
        }
    }
}

paz::RenderPass::RenderPass()
{
    initialize();

    _data = std::make_shared<Data>();
}

paz::RenderPass::RenderPass(const Framebuffer& fbo, const Shader& shader,
    BlendMode mode)
{
    initialize();

    _data = std::make_shared<Data>();

    _data->_fbo = fbo._data;
    _data->_shader = shader._data;
    _data->_blendMode = mode;
}

paz::RenderPass::RenderPass(const Shader& shader, BlendMode mode)
{
    initialize();

    _data = std::make_shared<Data>();

    _data->_shader = shader._data;
    _data->_blendMode = mode;
}

void paz::RenderPass::begin(const std::vector<LoadAction>& colorLoadActions,
    LoadAction depthLoadAction)
{
    glGetError();
    NextSlot = 0;
    if(_data->_fbo)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, _data->_fbo->_id);
        if(_data->_fbo->_width)
        {
            glViewport(0, 0, _data->_fbo->_width, _data->_fbo->_height);
        }
        else
        {
            glViewport(0, 0, Window::ViewportWidth(), Window::ViewportHeight());
        }
        for(std::size_t i = 0; i < colorLoadActions.size(); ++i)
        {
            if(colorLoadActions[i] == LoadAction::Clear)
            {
                glClearBufferfv(GL_COLOR, i, Transparent);
            }
            else if(colorLoadActions[i] != LoadAction::DontCare &&
                colorLoadActions[i] != LoadAction::Load)
            {
                throw std::runtime_error("Invalid color attachment load action"
                    ".");
            }
        }
        if(_data->_fbo->_depthStencilAttachment)
        {
            if(depthLoadAction == LoadAction::Clear)
            {
                if(!DepthMaskEnabled)
                {
                    DepthMaskEnabled = true;
                    glDepthMask(GL_TRUE);
                }
                glClear(GL_DEPTH_BUFFER_BIT);
            }
            else if(depthLoadAction != LoadAction::DontCare && depthLoadAction
                != LoadAction::Load)
            {
                throw std::runtime_error("Invalid depth attachment load action."
                    );
            }
        }
    }
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, Window::ViewportWidth(), Window::ViewportHeight());
        if(!colorLoadActions.empty() && colorLoadActions[0] == LoadAction::
            Clear)
        {
            glClearBufferfv(GL_COLOR, 0, Transparent);
        }
        if(depthLoadAction == LoadAction::Clear)
        {
            if(!DepthMaskEnabled)
            {
                DepthMaskEnabled = true;
                glDepthMask(GL_TRUE);
            }
            glClear(GL_DEPTH_BUFFER_BIT);
        }
    }

    if(_data->_blendMode == BlendMode::Disable)
    {
        if(BlendEnabled)
        {
            BlendEnabled = false;
            glDisable(GL_BLEND);
        }
    }
    else
    {
        if(!BlendEnabled)
        {
            BlendEnabled = true;
            glEnable(GL_BLEND);
        }
        if(_data->_blendMode == BlendMode::Additive)
        {
            glBlendFunc(GL_ONE, GL_ONE);
        }
        else if(_data->_blendMode == BlendMode::Blend)
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        // ...
        else
        {
            throw std::runtime_error("Invalid blending function.");
        }
    }

    if(!_data->_shader->_id)
    {
        throw std::runtime_error("Shader is not initialized.");
    }
    glUseProgram(_data->_shader->_id);
}

void paz::RenderPass::depth(DepthTestMode mode)
{
    if(mode == DepthTestMode::Disable)
    {
        if(DepthTestEnabled)
        {
            DepthTestEnabled = false;
            glDisable(GL_DEPTH_TEST);
        }
    }
    else
    {
        if(!DepthTestEnabled)
        {
            DepthTestEnabled = true;
            glEnable(GL_DEPTH_TEST);
        }
        if(mode < DepthTestMode::Never && DepthMaskEnabled)
        {
            DepthMaskEnabled = false;
            glDepthMask(GL_FALSE);
        }
        else if(!DepthMaskEnabled)
        {
            DepthMaskEnabled = true;
            glDepthMask(GL_TRUE);
        }
        if(mode == DepthTestMode::Never || mode == DepthTestMode::NeverNoMask)
        {
            glDepthFunc(GL_NEVER);
        }
        else if(mode == DepthTestMode::Less || mode == DepthTestMode::
            LessNoMask)
        {
            glDepthFunc(GL_LESS);
        }
        else if(mode == DepthTestMode::Equal || mode == DepthTestMode::
            EqualNoMask)
        {
            glDepthFunc(GL_EQUAL);
        }
        else if(mode == DepthTestMode::LessEqual || mode == DepthTestMode::
            LessEqualNoMask)
        {
            glDepthFunc(GL_LEQUAL);
        }
        else if(mode == DepthTestMode::Greater || mode == DepthTestMode::
            GreaterNoMask)
        {
            glDepthFunc(GL_GREATER);
        }
        else if(mode == DepthTestMode::NotEqual || mode == DepthTestMode::
            NotEqualNoMask)
        {
            glDepthFunc(GL_NOTEQUAL);
        }
        else if(mode == DepthTestMode::GreaterEqual || mode == DepthTestMode::
            GreaterEqualNoMask)
        {
            glDepthFunc(GL_GEQUAL);
        }
        else if(mode == DepthTestMode::Always || mode == DepthTestMode::
            AlwaysNoMask)
        {
            glDepthFunc(GL_ALWAYS);
        }
        else
        {
            throw std::runtime_error("Invalid depth testing function.");
        }
    }
}

void paz::RenderPass::end()
{
    const GLenum error = glGetError();
    if(error != GL_NO_ERROR)
    {
        throw std::runtime_error("Error in render pass: " + gl_error(error) +
            ".");
    }
}

void paz::RenderPass::cull(CullMode mode) const
{
    if(mode == CullMode::Disable)
    {
        glDisable(GL_CULL_FACE);
    }
    else if(mode == CullMode::Front)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
    }
    else if(mode == CullMode::Back)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
    else
    {
        throw std::runtime_error("Invalid culling mode.");
    }
}

void paz::RenderPass::read(const std::string& name, const Texture& tex) const
{
    glActiveTexture(GL_TEXTURE0 + NextSlot);
    glBindTexture(GL_TEXTURE_2D, tex._data->_id);
    uniform(name, NextSlot);
    ++NextSlot;
}

void paz::RenderPass::uniform(const std::string& name, int x) const
{
    CHECK_UNIFORM
    glUniform1i(std::get<0>(_data->_shader->_uniformIds.at(name)), x);
}

void paz::RenderPass::uniform(const std::string& name, int x, int y) const
{
    CHECK_UNIFORM
    glUniform2i(std::get<0>(_data->_shader->_uniformIds.at(name)), x, y);
}

void paz::RenderPass::uniform(const std::string& name, int x, int y, int z)
    const
{
    CHECK_UNIFORM
    glUniform3i(std::get<0>(_data->_shader->_uniformIds.at(name)), x, y, z);
}

void paz::RenderPass::uniform(const std::string& name, int x, int y, int z, int
    w) const
{
    CHECK_UNIFORM
    glUniform4i(std::get<0>(_data->_shader->_uniformIds.at(name)), x, y, z, w);
}

void paz::RenderPass::uniform(const std::string& name, const int* x, std::size_t
    size) const
{
    CHECK_UNIFORM
    switch(std::get<1>(_data->_shader->_uniformIds.at(name)))
    {
        case GL_INT:
            glUniform1iv(std::get<0>(_data->_shader->_uniformIds.at(name)),
                size, x);
            break;
        case GL_INT_VEC2:
            glUniform2iv(std::get<0>(_data->_shader->_uniformIds.at(name)),
                size/2, x);
            break;
        case GL_INT_VEC3:
            glUniform3iv(std::get<0>(_data->_shader->_uniformIds.at(name)),
                size/3, x);
            break;
        case GL_INT_VEC4:
            glUniform4iv(std::get<0>(_data->_shader->_uniformIds.at(name)),
                size/4, x);
            break;
        default:
            throw std::invalid_argument("Unsupported type " + std::to_string(
                std::get<1>(_data->_shader->_uniformIds.at(name))) +
                " for uniform \"" + name + "\".");
            break;
    }
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x) const
{
    CHECK_UNIFORM
    glUniform1ui(std::get<0>(_data->_shader->_uniformIds.at(name)), x);
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y) const
{
    CHECK_UNIFORM
    glUniform2ui(std::get<0>(_data->_shader->_uniformIds.at(name)), x, y);
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y, unsigned int z) const
{
    CHECK_UNIFORM
    glUniform3ui(std::get<0>(_data->_shader->_uniformIds.at(name)), x, y, z);
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y, unsigned int z, unsigned int w) const
{
    CHECK_UNIFORM
    glUniform4ui(std::get<0>(_data->_shader->_uniformIds.at(name)), x, y, z, w);
}

void paz::RenderPass::uniform(const std::string& name, const unsigned int* x,
    std::size_t size) const
{
    CHECK_UNIFORM
    switch(std::get<1>(_data->_shader->_uniformIds.at(name)))
    {
        case GL_UNSIGNED_INT:
            glUniform1uiv(std::get<0>(_data->_shader->_uniformIds.at(name)),
                size, x);
            break;
        case GL_UNSIGNED_INT_VEC2:
            glUniform2uiv(std::get<0>(_data->_shader->_uniformIds.at(name)),
                size/2, x);
            break;
        case GL_UNSIGNED_INT_VEC3:
            glUniform3uiv(std::get<0>(_data->_shader->_uniformIds.at(name)),
                size/3, x);
            break;
        case GL_UNSIGNED_INT_VEC4:
            glUniform4uiv(std::get<0>(_data->_shader->_uniformIds.at(name)),
                size/4, x);
            break;
        default:
            throw std::invalid_argument("Unsupported type " + std::to_string(
                std::get<1>(_data->_shader->_uniformIds.at(name))) +
                " for uniform \"" + name + "\".");
            break;
    }
}

void paz::RenderPass::uniform(const std::string& name, float x) const
{
    CHECK_UNIFORM
    glUniform1f(std::get<0>(_data->_shader->_uniformIds.at(name)), x);
}

void paz::RenderPass::uniform(const std::string& name, float x, float y) const
{
    CHECK_UNIFORM
    glUniform2f(std::get<0>(_data->_shader->_uniformIds.at(name)), x, y);
}

void paz::RenderPass::uniform(const std::string& name, float x, float y, float
    z) const
{
    CHECK_UNIFORM
    glUniform3f(std::get<0>(_data->_shader->_uniformIds.at(name)), x, y, z);
}

void paz::RenderPass::uniform(const std::string& name, float x, float y, float
    z, float w) const
{
    CHECK_UNIFORM
    glUniform4f(std::get<0>(_data->_shader->_uniformIds.at(name)), x, y, z, w);
}

void paz::RenderPass::uniform(const std::string& name, const float* x, std::
    size_t size) const
{
    CHECK_UNIFORM
    switch(std::get<1>(_data->_shader->_uniformIds.at(name)))
    {
        case GL_FLOAT:
            glUniform1fv(std::get<0>(_data->_shader->_uniformIds.at(name)),
                size, x);
            break;
        case GL_FLOAT_VEC2:
            glUniform2fv(std::get<0>(_data->_shader->_uniformIds.at(name)),
                size/2, x);
            break;
        case GL_FLOAT_VEC3:
            glUniform3fv(std::get<0>(_data->_shader->_uniformIds.at(name)),
                size/3, x);
            break;
        case GL_FLOAT_VEC4:
            glUniform4fv(std::get<0>(_data->_shader->_uniformIds.at(name)),
                size/4, x);
            break;
        case GL_FLOAT_MAT2:
            glUniformMatrix2fv(std::get<0>(_data->_shader->_uniformIds.at(
                name)), size/4, GL_FALSE, x);
            break;
        case GL_FLOAT_MAT3:
            glUniformMatrix3fv(std::get<0>(_data->_shader->_uniformIds.at(
                name)), size/9, GL_FALSE, x);
            break;
        case GL_FLOAT_MAT4:
            glUniformMatrix4fv(std::get<0>(_data->_shader->_uniformIds.at(
                name)), size/16, GL_FALSE, x);
            break;
        default:
            throw std::invalid_argument("Unsupported type " + std::to_string(
                std::get<1>(_data->_shader->_uniformIds.at(name))) +
                " for uniform \"" + name + "\".");
            break;
    }
}

void paz::RenderPass::primitives(PrimitiveType type, const VertexBuffer&
    vertices) const
{
    check_attributes(vertices._data->_types, _data->_shader->_attribTypes);

    glBindVertexArray(vertices._data->_id);
    glDrawArrays(primitive_type(type), 0, vertices._data->_numVertices);
}

void paz::RenderPass::indexed(PrimitiveType type, const VertexBuffer& vertices,
    const IndexBuffer& indices) const
{
    check_attributes(vertices._data->_types, _data->_shader->_attribTypes);

    glBindVertexArray(vertices._data->_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices._data->_id);
    glDrawElements(primitive_type(type), indices._data->_numIndices,
        GL_UNSIGNED_INT, 0);
}

paz::Framebuffer paz::RenderPass::framebuffer() const
{
    Framebuffer temp;
    temp._data = _data->_fbo;
    return temp;
}

#endif
