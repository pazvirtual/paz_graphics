#include "PAZ_Graphics"
#include <cmath>

std::array<float, 16> paz::perspective(float yFov, float ratio, float zNear,
    float zFar)
{
    const float tanHalfFov = std::tan(0.5*yFov);
    std::array<float, 16> res = {};
    res[0] = 1.f/(ratio*tanHalfFov);
    res[5] = 1.f/tanHalfFov;
    res[10] = (zNear + zFar)/(zNear - zFar);
    res[11] = -1.f;
    res[14] = 2.f*zFar*zNear/(zNear - zFar);
    return res;
}

std::array<float, 16> paz::ortho(const float left, const float right, const
    float bottom, const float top, const float zNear, const float zFar)
{
    std::array<float, 16> res = {};
    res[0] = 2.f/(right - left);
    res[5] = 2.f/(top - bottom);
    res[10] = 1.f/(zFar - zNear);
    res[12] = -(right + left)/(right - left);
    res[13] = -(top + bottom)/(top - bottom);
    res[14] = -zNear/(zFar - zNear);
    res[15] = 1.f;
    return res;
}

#if 0
std::array<float, 16> paz::translation(const std::array<float, 3>& delta)
{
    std::array<float, 16> res = {};
    for(std::size_t i = 0; i < 4; ++i)
    {
        res[5*i] = 1.0;
    }
    res[12] = delta(0);
    res[13] = delta(1);
    res[14] = delta(2);
    return res;
}

std::array<float, 16> paz::rotation(const paz::vec3 axisAngle)
{
    const double angle = paz::norm(axisAngle);
    if(!angle)
    {
        std::array<float, 16> res = {};
        for(std::size_t i = 0; i < 4; ++i)
        {
            res[5*i] = 1.0;
        }
        return res;
    }
    const paz::vec3 axis = axisAngle/angle;
    const double c = cos(angle);
    const double s = sin(angle);
    const double t = 1.0 - c;
    return std::array<float, 16>
    {
        static_cast<float>(t*axis(0)*axis(0) + c),
        static_cast<float>(t*axis(0)*axis(1) + axis(2)*s),
        static_cast<float>(t*axis(0)*axis(2) - axis(1)*s),
        0.0,
        static_cast<float>(t*axis(0)*axis(1) - axis(2)*s),
        static_cast<float>(t*axis(1)*axis(1) + c),
        static_cast<float>(t*axis(1)*axis(2) + axis(0)*s),
        0.0,
        static_cast<float>(t*axis(0)*axis(2) + axis(1)*s),
        static_cast<float>(t*axis(1)*axis(2) - axis(0)*s),
        static_cast<float>(t*axis(2)*axis(2) + c),
        0.0,
        0.0,
        0.0,
        0.0,
        1.0
    };
}

std::array<float, 16> paz::transform(const paz::vec3 delta, const paz::vec3
    axisAngle)
{
    const double angle = paz::norm(axisAngle);
    if(!angle)
    {
        return translation(delta);
    }
    const paz::vec3 axis = axisAngle/angle;
    const double c = cos(angle);
    const double s = sin(angle);
    const double t = 1.0 - c;

    return std::array<float, 16>
    {
        static_cast<float>(t*axis(0)*axis(0) + c),
        static_cast<float>(t*axis(0)*axis(1) + axis(2)*s),
        static_cast<float>(t*axis(0)*axis(2) - axis(1)*s),
        0.0,
        static_cast<float>(t*axis(0)*axis(1) - axis(2)*s),
        static_cast<float>(t*axis(1)*axis(1) + c),
        static_cast<float>(t*axis(1)*axis(2) + axis(0)*s),
        0.0,
        static_cast<float>(t*axis(0)*axis(2) + axis(1)*s),
        static_cast<float>(t*axis(1)*axis(2) - axis(0)*s),
        static_cast<float>(t*axis(2)*axis(2) + c),
        0.0,
        static_cast<float>(delta(0)),
        static_cast<float>(delta(1)),
        static_cast<float>(delta(2)),
        1.0
    };
}
#endif

std::array<float, 16> paz::transform(const std::array<float, 3>& delta, const
    std::array<float, 9>& rot)
{
    return {  rot[0],   rot[1],   rot[2], 0.f,
              rot[3],   rot[4],   rot[5], 0.f,
              rot[6],   rot[7],   rot[8], 0.f,
            delta[0], delta[1], delta[2], 1.f};
}
