#ifndef PAZ_GRAPHICS_VERTEX_BUFFER_HPP
#define PAZ_GRAPHICS_VERTEX_BUFFER_HPP

#include <vector>

namespace paz
{
    template<typename T>
    void check_size(int dim, std::size_t& n, const std::vector<T>& d)
    {
        if(dim != 1 && dim != 2 && dim != 4)
        {
            throw std::runtime_error("Vertex attribute dimensions must be 1, 2,"
                " or 4.");
        }
        const std::size_t m = d.size()/dim;
        if(!n)
        {
            n = m;
        }
        else if(m != n)
        {
            throw std::runtime_error("Number of vertices for each attribute mus"
                "t match.");
        }
    }
}

#endif
