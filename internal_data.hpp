#ifndef PAZ_GRAPHICS_INTERNAL_DATA_HPP
#define PAZ_GRAPHICS_INTERNAL_DATA_HPP

#include "detect_os.hpp"
#include "PAZ_Graphics"

struct paz::Texture::Data
{
#ifdef PAZ_MACOS
    void* _sampler = nullptr;
    void* _texture = nullptr;
    int _numChannels;
    int _numBits;
    DataType _type;
    MinMagFilter _minFilter;
    MinMagFilter _magFilter;
#else
    unsigned int _id = 0;
    int _internalFormat;
    unsigned int _format;
    unsigned int _type;
#endif
};

struct paz::VertexBuffer::Data
{
#ifdef PAZ_MACOS
    std::vector<void*> _buffers;
#else
    unsigned int _id = 0;
    std::vector<unsigned int> _ids;
    std::vector<unsigned int> _types;
#endif
};

struct paz::IndexBuffer::Data
{
#ifdef PAZ_MACOS
    void* _data = nullptr;
#else
    unsigned int _id = 0;
#endif
};

struct paz::Framebuffer::Data
{
#ifdef PAZ_MACOS
        std::vector<const RenderTarget*> _colorAttachments;
        const DepthStencilTarget* _depthAttachment = nullptr;
#else
        unsigned int _id = 0;
        int _numTextures = 0;
        bool _hasDepthAttachment = false;
#endif
};

struct paz::ShaderFunctionLibrary::Data
{
#ifdef PAZ_MACOS
        std::unordered_map<std::string, void*> _verts;
        std::unordered_map<std::string, void*> _frags;
#else
        std::unordered_map<std::string, unsigned int> _vertexIds;
        std::unordered_map<std::string, unsigned int> _fragmentIds;
#endif
};

struct paz::Shader::Data
{
#ifdef PAZ_MACOS
    void* _vert = nullptr;
    void* _frag = nullptr;
#else
    unsigned int _id = 0;
    // uniformIDs[name] = (id, type, size)
    std::unordered_map<std::string, std::tuple<unsigned int, unsigned int, int>>
        _uniformIds;
    // attribTypes[location] = type (array attributes are not supported)
    std::unordered_map<unsigned int, unsigned int> _attribTypes;
#endif
};

struct paz::RenderPass::Data
{
#ifdef PAZ_MACOS
    void* _pipelineState = nullptr;
    void* _renderEncoder = nullptr;
    std::unordered_map<std::string, int> _vertexArgs;
    std::unordered_map<std::string, int> _fragmentArgs;
#else
    const Shader* _shader = nullptr;
    BlendMode _blendMode = BlendMode::Disable;
#endif
};

#endif
