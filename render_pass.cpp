#include "PAZ_Graphics"

#ifndef PAZ_MACOS

#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

#define CASE_STRING(x) case x: return #x;

static constexpr float Transparent[] = {0.f, 0.f, 0.f, 0.f};
static int NextSlot = 0;

static bool DepthTestEnabled = false;

static GLenum primitive_type(paz::RenderPass::PrimitiveType t)
{
    //
    if(t == paz::RenderPass::PrimitiveType::Triangles)
    {
        return GL_TRIANGLES;
    }
    //

    throw std::runtime_error("Invalid primitive type.");
}

static std::string gl_error(GLenum error)
{
    switch(error)
    {
        CASE_STRING(GL_NO_ERROR)
        CASE_STRING(GL_INVALID_ENUM)
        CASE_STRING(GL_INVALID_VALUE)
        CASE_STRING(GL_INVALID_OPERATION)
        CASE_STRING(GL_INVALID_FRAMEBUFFER_OPERATION)
        CASE_STRING(GL_OUT_OF_MEMORY)
#ifdef GL_STACK_UNDERFLOW
        CASE_STRING(GL_STACK_UNDERFLOW)
#endif
#ifdef GL_STACK_OVERFLOW
        CASE_STRING(GL_STACK_OVERFLOW)
#endif
    }
    return "Error code " + std::to_string(error) + " not recognized";
}

paz::RenderPass::~RenderPass()
{
    _fbo = nullptr;
    _shader = nullptr;
}

paz::RenderPass::RenderPass(const Framebuffer& fbo, const Shader& shader)
{
    _fbo = &fbo;
    _shader = &shader;
}

paz::RenderPass::RenderPass(const Shader& shader)
{
    _shader = &shader;
}

void paz::RenderPass::begin(const std::vector<LoadAction>& colorLoadActions,
    LoadAction depthLoadAction)
{
    glGetError();
    NextSlot = 0;
    if(_fbo)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo->_id);
        //glViewport(0, 0, _fbo->width(), _fbo->height());
        glViewport(0, 0, Window::ViewportWidth(), Window::ViewportHeight());//TEMP
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
        if(_fbo->_hasDepthAttachment)
        {
            if(depthLoadAction == LoadAction::Clear)
            {
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
            glClear(GL_DEPTH_BUFFER_BIT);
        }
    }
    _shader->use();
}

void paz::RenderPass::depth(DepthTestMode depthMode)
{
    if(depthMode == DepthTestMode::Disable)
    {
        if(DepthTestEnabled)
        {
            glDisable(GL_DEPTH_TEST);
        }
    }
    else
    {
        if(!DepthTestEnabled)
        {
            glEnable(GL_DEPTH_TEST);
        }
        if(depthMode == DepthTestMode::Never)
        {
            glDepthFunc(GL_NEVER);
        }
        else if(depthMode == DepthTestMode::Less)
        {
            glDepthFunc(GL_LESS);
        }
        else if(depthMode == DepthTestMode::Equal)
        {
            glDepthFunc(GL_EQUAL);
        }
        else if(depthMode == DepthTestMode::LessEqual)
        {
            glDepthFunc(GL_LEQUAL);
        }
        else if(depthMode == DepthTestMode::Greater)
        {
            glDepthFunc(GL_GREATER);
        }
        else if(depthMode == DepthTestMode::NotEqual)
        {
            glDepthFunc(GL_NOTEQUAL);
        }
        else if(depthMode == DepthTestMode::GreaterEqual)
        {
            glDepthFunc(GL_GEQUAL);
        }
        else if(depthMode == DepthTestMode::Always)
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
    glBindTexture(GL_TEXTURE_2D, tex._id);
    _shader->uniform(name, NextSlot);
    ++NextSlot;
}

void paz::RenderPass::uniform(const std::string& name, int x) const
{
    _shader->uniform(name, x);
}

void paz::RenderPass::uniform(const std::string& name, int x, int y) const
{
    _shader->uniform(name, x, y);
}

void paz::RenderPass::uniform(const std::string& name, int x, int y, int z)
    const
{
    _shader->uniform(name, x, y, z);
}

void paz::RenderPass::uniform(const std::string& name, int x, int y, int z, int
    w) const
{
    _shader->uniform(name, x, y, z, w);
}

void paz::RenderPass::uniform(const std::string& name, const int* x, int n)
    const
{
    _shader->uniform(name, x, n);
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x) const
{
    _shader->uniform(name, x);
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y) const
{
    _shader->uniform(name, x, y);
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y, unsigned int z) const
{
    _shader->uniform(name, x, y, z);
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y, unsigned int z, unsigned int w) const
{
    _shader->uniform(name, x, y, z, w);
}

void paz::RenderPass::uniform(const std::string& name, const unsigned int* x,
    int n) const
{
    _shader->uniform(name, x, n);
}

void paz::RenderPass::uniform(const std::string& name, float x) const
{
    _shader->uniform(name, x);
}

void paz::RenderPass::uniform(const std::string& name, float x, float y) const
{
    _shader->uniform(name, x, y);
}

void paz::RenderPass::uniform(const std::string& name, float x, float y, float
    z) const
{
    _shader->uniform(name, x, y, z);
}

void paz::RenderPass::uniform(const std::string& name, float x, float y, float
    z, float w) const
{
    _shader->uniform(name, x, y, z, w);
}

void paz::RenderPass::uniform(const std::string& name, const float* x, int n)
    const
{
    _shader->uniform(name, x, n);
}

void paz::RenderPass::primitives(PrimitiveType type, const VertexBuffer&
    vertices, int offset) const
{
    glBindVertexArray(vertices._id);
    glDrawArrays(primitive_type(type), offset, vertices._numVertices);
}

void paz::RenderPass::indexed(PrimitiveType type, const VertexBuffer& vertices,
    const IndexBuffer& indices, int offset) const
{
    glBindVertexArray(vertices._id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices._id);
    glDrawElements(primitive_type(type), indices._numIndices, GL_UNSIGNED_INT,
        (void*)(std::intptr_t)offset);
}

#endif
