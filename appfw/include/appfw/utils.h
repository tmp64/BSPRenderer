#ifndef APPFW_COMMAND_UTILS_H
#define APPFW_COMMAND_UTILS_H
#include <vector>
#include <string>

namespace appfw {

using ParsedCommand = std::vector<std::string>;

namespace utils {

/**
 * Parses a string into a vector of strings.
 * Handles quotes, strips whitespaces.
 */
ParsedCommand parseCommand(const std::string &cmd);

//----------------------------------------------------------------

/**
 * A dummy templated struct to be used with static_assert and templates.
 * Use like this: static_assert(FalseT<T>::value, "Bad")
 */
template <typename T>
struct FalseT : std::false_type {};

/**
 * An uncopyable class.
 * Inherit from it to make your class uncopyable as well.
 */
class NoCopy {
public:
    NoCopy() = default;
    NoCopy(const NoCopy &) = delete;
    NoCopy &operator=(const NoCopy &) = delete;
};

//----------------------------------------------------------------

/**
 * Converts a value of type T into a string.
 */
template <typename T>
inline std::string valToString(const T &val) {
    return std::to_string(val);
}

/**
 * Returns string unchanged.
 */
template<>
inline std::string valToString<std::string>(const std::string &val) {
    return val;
}

//----------------------------------------------------------------

template <typename T>
bool stringToVal(const std::string &str, T &val);

template <typename T>
constexpr const char *typeNameToString() {
    static_assert(FalseT<T>::value, "typeNameToString: template instantiation for T was not found");
}

template <>
constexpr const char *typeNameToString<int>() {
    return "int";
}

template <>
constexpr const char *typeNameToString<float>() {
    return "float";
}

template <>
constexpr const char *typeNameToString<std::string>() {
    return "string";
}

template <>
constexpr const char *typeNameToString<bool>() {
    return "bool";
}

} // namespace utils

} // namespace appfw

#endif