#include "opengl2metal.hpp"
#include <regex>

std::string paz::process_sig(const std::string& sig)
{
    const std::size_t open = sig.find('(');
    const std::string front = sig.substr(0, open);
    std::string args = "thread " + sig.substr(open + 1, sig.size() - open - 2);
    args = std::regex_replace(args, std::regex("\\b(in)?out\\s(\\S+)\\s"),
        "$2& ");
    args = std::regex_replace(args, std::regex("\\bin\\s+"), "");
    args = std::regex_replace(args, std::regex(",\\s"), ", thread ");
    return front + "(" + args +")";
}
