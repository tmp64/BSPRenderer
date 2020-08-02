#include <appfw/utils.h>

appfw::ParsedCommand appfw::utils::parseCommand(const std::string &cmd) {
    std::vector<std::string> args;
    size_t i = 0;

    // Skip spaces
    while (cmd[i] == ' ')
        i++;

    constexpr size_t NO_ARG = std::numeric_limits<size_t>::max();

    // Parse the string
    size_t argStart = i;
    bool isInQuotes = false;
    for (; i <= cmd.size(); i++) {
        char c = cmd[i];

        if (argStart == NO_ARG) {
            // Skip spaces
            if (c == ' ' || c == '\0')
                continue;
            else
                argStart = i;
        }

        if (!isInQuotes) {
            if (c == ' ' || c == '\0') {
                size_t len = i - argStart;
                args.push_back(cmd.substr(argStart, len));
                argStart = NO_ARG;
            } else if (c == '"') {
                isInQuotes = true;
                argStart = i + 1;
            }
        } else {
            if (c == '"' || c == '\0') {
                size_t len = i - argStart;
                args.push_back(cmd.substr(argStart, len));
                argStart = NO_ARG;
                isInQuotes = false;
            }
        }
    }

    return args;
}

//----------------------------------------------------------------

template <typename T>
bool appfw::utils::stringToVal(const std::string &str, T &val) {
    static_assert(FalseT<T>::value, "T cannot be made from string");
    return false;
}

template<>
bool appfw::utils::stringToVal(const std::string &str, int &val) {
    try {
        val = std::stoi(str);
        return true;
    }
    catch (...) { return false; }
}

template <>
bool appfw::utils::stringToVal(const std::string &str, float &val) {
    try {
        val = std::stof(str);
        return true;
    } catch (...) { return false; }
}

template <>
bool appfw::utils::stringToVal(const std::string &str, std::string &val) {
    val = str;
    return true;
}

template <>
bool appfw::utils::stringToVal(const std::string &str, bool &val) {
    int iVal = 0;
    if (!stringToVal(str, iVal)) {
        return false;
    }
    val = !!iVal;
    return true;
}
