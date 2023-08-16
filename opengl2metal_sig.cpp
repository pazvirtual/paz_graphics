#include "opengl2metal.hpp"
#include <regex>

std::string paz::process_sig(const std::string& sig, std::unordered_set<std::
    string>& argNames)
{
    if(sig.find('[') != std::string::npos)
    {
        throw std::runtime_error("Arrays in private function signatures are not"
            " supported.");
    }
    const std::size_t open = sig.find('(');
    if(open == std::string::npos)
    {
        throw std::logic_error("Invalid signature for private function.");
    }
    auto start = sig.begin() + open;
    std::smatch res;
    while(std::regex_search(start, sig.end(), res, std::regex("\\s*([A-Za-z_][A"
        "-Za-z_0-9]*)\\s*[,)]")))
    {
        argNames.insert(res[1]);
        start = res.suffix().first;
    }
    const std::string front = sig.substr(0, open);
    std::string args = "thread " + sig.substr(open + 1, sig.size() - open - 2);
    args = std::regex_replace(args, std::regex("\\b(in)?out\\s(\\S+)\\s"),
        "$2& ");
    args = std::regex_replace(args, std::regex("\\bin\\s+"), "");
    args = std::regex_replace(args, std::regex(",\\s"), ", thread ");
    return front + "(" + args +")";
}
