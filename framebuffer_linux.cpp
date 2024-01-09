#include "detect_os.hpp"

#ifdef PAZ_LINUX

#include "PAZ_Graphics"
#include "internal_data.hpp"
#include "gl_core_4_1.h"

#define CASE_STRING(x) case x: return #x;

static constexpr GLenum Attachments[] = {GL_COLOR_ATTACHMENT0,
    GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
    GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6,
    GL_COLOR_ATTACHMENT7};

static std::string framebuffer_status(GLenum status) noexcept
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
        default: return "Status code " + std::to_string(status) + " not recogni"
            "zed";
    }
}

paz::Framebuffer::Data::~Data()
{
    glDeleteFramebuffers(1, &_id);
}

paz::Framebuffer::Data::Data()
{
    glGenFramebuffers(1, &_id);
}

void paz::Framebuffer::attach(const RenderTarget& target)
{
    if(target._data->_format == TextureFormat::RGBA8UNorm_sRGB)
    {
        throw std::runtime_error("Drawing to sRGB textures is not supported.");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, _data->_id);
    if(target._data->_format == TextureFormat::Depth16UNorm || target._data->
        _format == TextureFormat::Depth32Float)
    {
        _data->_depthStencilAttachment = target._data; //TEMP
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target._data->
            _id, 0);
    }
    else
    {
        _data->_colorAttachments.push_back(target._data); //TEMP
        glFramebufferTexture(GL_FRAMEBUFFER, Attachments[_data->_numTextures],
            target._data->_id, 0);
        glDrawBuffers(++_data->_numTextures, Attachments);
    }
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error("Framebuffer incomplete: " +
            framebuffer_status(status) + ".");
    }
}

#endif
