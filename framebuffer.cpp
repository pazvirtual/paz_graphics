#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "internal_data.hpp"
#include "window.hpp"
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

paz::Framebuffer::Data::~Data()
{
    glDeleteFramebuffers(1, &_id);
}

paz::Framebuffer::Data::Data()
{
    glGenFramebuffers(1, &_id);
}

paz::Framebuffer::Framebuffer()
{
    initialize();

    _data = std::make_shared<Data>();
}

void paz::Framebuffer::attach(const RenderTarget& target)
{
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
    if(!target._data->_scale) //TEMP
    {
        _data->_width = target.width();
        _data->_height = target.height();
    }

}

paz::Texture paz::Framebuffer::colorAttachment(std::size_t i) const
{
    if(i >= _data->_colorAttachments.size())
    {
        throw std::runtime_error("Color attachment " + std::to_string(i) + " is"
            " out of bounds.");
    }
    Texture temp;
    temp._data = _data->_colorAttachments[i];
    return temp;
}

paz::Texture paz::Framebuffer::depthStencilAttachment() const
{
    if(!_data->_depthStencilAttachment)
    {
        throw std::runtime_error("Framebuffer has no depth/stencil attachment."
            );
    }
    Texture temp;
    temp._data = _data->_depthStencilAttachment;
    return temp;
}

#endif
