#include "PAZ_Graphics"

#ifndef PAZ_MACOS

#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

static constexpr GLenum Slots[] = {GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2,
    GL_TEXTURE3, GL_TEXTURE4, GL_TEXTURE5, GL_TEXTURE6, GL_TEXTURE7};

paz::TextureBase::TextureBase() {}

paz::TextureBase::~TextureBase()
{
    if(_id)
    {
        glDeleteTextures(1, &_id);
    }
}

void paz::TextureBase::activate(std::size_t slot) const
{
    glActiveTexture(Slots[slot]);
    glBindTexture(GL_TEXTURE_2D, _id);
}

#endif
