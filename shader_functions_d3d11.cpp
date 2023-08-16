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
}

paz::FragmentFunction::FragmentFunction()
{
}

paz::VertexFunction::VertexFunction(const std::string& src)
{
}

paz::FragmentFunction::FragmentFunction(const std::string& src)
{
}

#endif
