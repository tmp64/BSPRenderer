#ifndef APPFW_COMMAND_UTILS_H
#define APPFW_COMMAND_UTILS_H
#include <array>
#include <vector>
#include <string>
#include <appfw/dbg.h>

namespace appfw {

using ParsedCommand = std::vector<std::string>;

//----------------------------------------------------------------

/**
 * An object that references a sequence of N objects in memory.
 * Really basic non-standart implementation of std::span from C++20.
 */
template <typename T>
class span {
public:
    span() = default;

    inline span(T *ptr, size_t size) noexcept {
        m_Ptr = ptr;
        m_Size = size;
    }

    inline span(std::vector<T> &cont) noexcept : span(cont.data(), cont.size()) {}

    template <size_t N>
    inline span(std::array<T, N> &cont) noexcept : span(cont.data(), N) {}

    inline T *begin() const noexcept { return m_Ptr; }
    inline T *end() const noexcept { return m_Ptr + m_Size; }

    inline T &front() const noexcept { return *m_Ptr; }
    inline T &back() const noexcept { return *(m_Ptr + m_Size - 1); }

    inline T &operator[](size_t idx) const noexcept {
        AFW_ASSERT_MSG(idx >= 0 && idx < m_Size, "span index out of range");
        return *(m_Ptr + idx);
    }

    inline T *data() const noexcept { return m_Ptr; }

    inline size_t size() const noexcept { return m_Size; }
    inline size_t size_bytes() const noexcept { return m_Size * sizeof(T); }

    inline bool empty() const noexcept { return m_Size == 0; }

    inline span<T> first(size_t n) const noexcept {
        AFW_ASSERT_MSG(n <= m_Size, "subspan size out of range");
        return span(m_Ptr, n);
    }

    inline span<T> last(size_t n) const noexcept {
        AFW_ASSERT_MSG(n <= m_Size, "subspan size out of range");
        return span(m_Ptr + m_Size - n, n);
    }

    inline span<T> subspan(size_t offset, size_t count) const noexcept {
        AFW_ASSERT_MSG(count + offset <= m_Size, "subspan out of range");
        return span(m_Ptr + offset, count);
    }

private:
    T *m_Ptr = nullptr;
    size_t m_Size = 0;
};

//----------------------------------------------------------------

namespace utils {

/**
 * Parses a string into a vector of strings.
 * Handles quotes, strips whitespaces.
 */
ParsedCommand parseCommand(const std::string &cmd);

/**
 * Converts a parsed command to a string. All arguments are quoted.
 */
std::string commandToString(const ParsedCommand &cmd);

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