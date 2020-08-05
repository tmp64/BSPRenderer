#ifndef APPFW_BINARY_FILE_H
#define APPFW_BINARY_FILE_H
#include <appfw/utils.h>
#include <fstream>

namespace appfw {

/**
 * BinaryWriter - a helper class that can write any binary data into a file.
 * On error always throws.
 */
class BinaryWriter : appfw::utils::NoCopy {
public:
    /**
     * Construct a closed binary writer.
     */
    BinaryWriter() = default;

    /**
     * Constructs and calls open(path)
     */
    BinaryWriter(const std::string &path);

    /**
     * Opens file for writing. On error throws std::runtime_error.
     */
    void open(const std::string &path);

    /**
     * Closes open file.
     */
    void close();

    /**
     * Whether or not the file is open.
     */
    bool isOpen();

    /**
     * Writes `len` bytes into the file.
     * @param   data    Data to be written.
     * @param   len     Length of the data.
     */
    void writeBytes(const char *data, size_t len);

    /**
     * Writes an array of T.
     */
    template <typename T>
    inline void writeArray(appfw::span<T> data) {
        static_assert(std::is_standard_layout<T>::value, "T is not a standart layout type");
        writeBytes((const char *)data.data(), data.size() * sizeof(T));
    }

    /**
     * Writes an element of type T.
     */
    template <typename T>
    inline void write(const T &data) {
        static_assert(std::is_standard_layout<T>::value, "T is not a standart layout type");
        writeBytes((const char *)&data, sizeof(T));
    }

private:
    std::ofstream m_File;
};

/**
 * BinaryReader - a helper class that can read any binary data from a file.
 * On error always throws.
 */
class BinaryReader : appfw::utils::NoCopy {
public:
    /**
     * Construct a closed binary reader.
     */
    BinaryReader() = default;

    /**
     * Constructs and calls open(path)
     */
    BinaryReader(const std::string &path);

    /**
     * Opens file for reading. On error throws std::runtime_error.
     */
    void open(const std::string &path);

    /**
     * Closes open file.
     */
    void close();

    /**
     * Whether or not the file is open.
     */
    bool isOpen();

    /**
     * Returns size of the file in bytes.
     */
    size_t getFileSize();

    /**
     * Reads `len` bytes from the file.
     * @param   data    Array to store the bytes.
     * @param   len     Length of the data to read.
     */
    void readBytes(char *data, size_t len);

    /**
     * Reads an array of T.
     */
    template <typename T>
    inline void readArray(appfw::span<T> data) {
        static_assert(std::is_standard_layout<T>::value, "T is not a standart layout type");
        readBytes((char *)data.data(), data.size() * sizeof(T));
    }

    /**
     * Reads an element of type T.
     */
    template <typename T>
    inline void read(T &data) {
        static_assert(std::is_standard_layout<T>::value, "T is not a standart layout type");
        readBytes((char *)&data, sizeof(T));
    }

private:
    std::ifstream m_File;
    size_t m_iSize = 0;
};

} // namespace appfw

#endif
