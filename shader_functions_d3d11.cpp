#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "util_d3d11.hpp"
//#include "opengl2d3d.hpp"
#include "internal_data.hpp"
#include "window.hpp"

paz::VertexFunction::Data::~Data()
{
}

paz::FragmentFunction::Data::~Data()
{
}

paz::VertexFunction::VertexFunction()
{
    initialize();

    _data = std::make_shared<Data>();
}

paz::FragmentFunction::FragmentFunction()
{
    initialize();

    _data = std::make_shared<Data>();
}

paz::VertexFunction::VertexFunction(const std::string& src)
{
    initialize();

    _data = std::make_shared<Data>();
}

paz::FragmentFunction::FragmentFunction(const std::string& src)
{
    initialize();

    _data = std::make_shared<Data>();
}

#endif
