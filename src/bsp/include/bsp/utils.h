#ifndef BSP_UTILS_H
#define BSP_UTILS_H
#include <vector>
#include <string>
#include <string_view>

namespace bsp {

inline std::vector<std::string> parseWadListString(std::string_view wads) {
    std::vector<std::string> wadList;

    if (wads.empty()) {
        return wadList;
    }

    size_t oldsep = static_cast<size_t>(0) - 1;
    size_t sep = 0;

    auto fnLoadWad = [&](std::string_view wadpath) {
        if (wadpath.empty()) {
            return;
        }

        // Find last '/' or '\'
        size_t pos = (size_t)0 - 1;

        size_t slashpos = wadpath.find_last_of('/');
        if (slashpos != wadpath.npos)
            pos = slashpos;

        size_t backslashpos = wadpath.find_last_of('\\');
        if (backslashpos != wadpath.npos && (slashpos == wadpath.npos || backslashpos > slashpos))
            pos = backslashpos;

        std::string_view wadname = wadpath.substr(pos + 1);
        wadList.push_back(std::string(wadname));
    };

    while ((sep = wads.find(';', sep)) != wads.npos) {
        std::string_view wadpath = wads.substr(oldsep + 1, sep - oldsep - 1);
        fnLoadWad(wadpath);
        oldsep = sep;
        sep++;
    }

    fnLoadWad(wads.substr(oldsep + 1));

    return wadList;
}

} // namespace bsp

#endif
