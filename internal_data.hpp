#ifndef PAZ_GRAPHICS_INTERNAL_DATA_HPP
#define PAZ_GRAPHICS_INTERNAL_DATA_HPP

#include "detect_os.hpp"
#ifndef PAZ_MACOS
#include "shader.hpp"
#endif
#include "PAZ_Graphics"
#include <unordered_map>
#include <unordered_set>

struct paz::Texture::Data
{
#ifdef PAZ_MACOS
    void* _sampler = nullptr;
    void* _texture = nullptr;
#else
    unsigned int _id = 0;
#endif
    int _width = 0;
    int _height = 0;
    TextureFormat _format;
    MinMagFilter _minFilter;
    MinMagFilter _magFilter;
    MipmapFilter _mipFilter = MipmapFilter::None;
    WrapMode _wrapS;
    WrapMode _wrapT;
    bool _isRenderTarget = false;
    double _scale = 1.;
    ~Data();
    void init(const void* data = nullptr);
    void ensureMipmaps();
    void resize(int width, int height);
};

struct paz::VertexBuffer::Data
{
#ifdef PAZ_MACOS
    std::vector<void*> _buffers;
    void* _lineLoopIndices = nullptr;
    void* _triangleFanIndices = nullptr;
#else
    unsigned int _id = 0;
    std::vector<unsigned int> _ids;
    std::vector<unsigned int> _types;
    Data();
#endif
    std::size_t _numVertices = 0;
    ~Data();
    void checkSize(int dim, std::size_t size);
};

struct paz::InstanceBuffer::Data
{
#ifdef PAZ_MACOS
    std::vector<std::vector<unsigned char>> _buffers;
#else
    unsigned int _id = 0;
    std::vector<unsigned int> _ids;
    std::vector<unsigned int> _types;
    std::vector<int> _dims;
    Data();
    ~Data();
    void addAttribute(int dim, DataType type);
#endif
    std::size_t _numInstances = 0;
    void checkSize(int dim, std::size_t size);
};

struct paz::IndexBuffer::Data
{
#ifdef PAZ_MACOS
    void* _data = nullptr;
    void* _lineLoopIndices = nullptr;
    void* _triangleFanIndices = nullptr;
#else
    unsigned int _id = 0;
#endif
    std::size_t _numIndices = 0;
    ~Data();
};

struct paz::Framebuffer::Data
{
    std::vector<std::shared_ptr<Texture::Data>> _colorAttachments; //TEMP
    std::shared_ptr<Texture::Data> _depthStencilAttachment; //TEMP
    int _width = 0;
    int _height = 0;
    double _scale = 1.;
#ifndef PAZ_MACOS
    unsigned int _id = 0;
    int _numTextures = 0;
    ~Data();
    Data();
#endif
};

struct paz::VertexFunction::Data
{
#ifdef PAZ_MACOS
    void* _function = nullptr;
#else
    unsigned int _id = 0;
#endif
    ~Data();
};

struct paz::FragmentFunction::Data
{
#ifdef PAZ_MACOS
    void* _function = nullptr;
#else
    unsigned int _id = 0;
    std::unordered_map<unsigned int, unsigned int> _outputTypes;
#endif
    ~Data();
};

struct paz::RenderPass::Data
{
#ifdef PAZ_MACOS
    void* _pipelineState = nullptr;
    void* _renderEncoder = nullptr;
    std::unordered_map<std::string, int> _vertexArgs;
    std::unordered_map<std::string, int> _fragmentArgs;
    std::vector<std::size_t> _vertexAttributeStrides;
    std::shared_ptr<VertexFunction::Data> _vert;
    std::shared_ptr<FragmentFunction::Data> _frag;
    ~Data();
#else
    BlendMode _blendMode = BlendMode::Disable;
    ShaderData _shader;
#endif
    std::shared_ptr<Framebuffer::Data> _fbo;
};

#endif
