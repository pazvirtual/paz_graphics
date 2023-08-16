#include "PAZ_Graphics"

#ifndef PAZ_MACOS

#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

#define CASE_STRING(x) case x: return #x;

static constexpr GLenum Attachments[] = {GL_COLOR_ATTACHMENT0,
    GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
    GL_COLOR_ATTACHMENT4};

static std::string framebuffer_status(GLenum status)
{
    switch(status)
    {
        CASE_STRING(GL_FRAMEBUFFER_COMPLETE)
        CASE_STRING(GL_FRAMEBUFFER_UNDEFINED)
        CASE_STRING(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
        CASE_STRING(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
        CASE_STRING(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER)
        CASE_STRING(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER)
        CASE_STRING(GL_FRAMEBUFFER_UNSUPPORTED)
        CASE_STRING(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
        CASE_STRING(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS)
    }
    return "Status code " + std::to_string(status) + " not recognized";
}

void paz::Framebuffer::clean()
{
    glDeleteFramebuffers(1, &_id);
    _numTextures = 0;
    _hasDepthAttachment = false;
}

void paz::Framebuffer::attach(const RenderTarget& target)
{
    glBindFramebuffer(GL_FRAMEBUFFER, _id);
    glFramebufferTexture(GL_FRAMEBUFFER, Attachments[_numTextures], target._id,
        0);
    glDrawBuffers(++_numTextures, Attachments);
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error("Framebuffer incomplete: " +
            framebuffer_status(status) + ".");
    }
}

void paz::Framebuffer::attach(const DepthStencilTarget& target)
{
    glBindFramebuffer(GL_FRAMEBUFFER, _id);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target._id, 0);
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error("Framebuffer incomplete: " +
            framebuffer_status(status) + ".");
    }
    _hasDepthAttachment = true;
}

paz::Framebuffer::Framebuffer()
{
    glGenFramebuffers(1, &_id);
}

#endif
