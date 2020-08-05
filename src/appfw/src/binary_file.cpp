#include <appfw/binary_file.h>
#include <appfw/dbg.h>
#include <fmt/format.h>

appfw::BinaryWriter::BinaryWriter(const std::string &path) { open(path); }

void appfw::BinaryWriter::open(const std::string &path) {
    m_File.open(path, std::ios::out | std::ios::binary);

    if (m_File.fail()) {
        throw std::runtime_error(fmt::format("Failed to open {}: {}", path, strerror(errno)));
    }

    AFW_ASSERT(isOpen());
}

void appfw::BinaryWriter::close() { m_File.close(); }

bool appfw::BinaryWriter::isOpen() { return m_File.is_open(); }

void appfw::BinaryWriter::writeBytes(const char *data, size_t len) {
    AFW_ASSERT_MSG(isOpen(), "Can't write to a closed file");
    m_File.write(data, len);
    if (m_File.fail()) {
        throw std::runtime_error(fmt::format("Write error: {}", strerror(errno)));
    }
}

appfw::BinaryReader::BinaryReader(const std::string &path) { open(path); }

void appfw::BinaryReader::open(const std::string &path) {
    m_File.open(path, std::ios::in | std::ios::binary);

    if (m_File.fail()) {
        throw std::runtime_error(fmt::format("Failed to open {}: {}", path, strerror(errno)));
    }
    
    AFW_ASSERT(isOpen());

    m_File.seekg(0, m_File.end);
    m_iSize = m_File.tellg();
    m_File.seekg(0, m_File.beg);
}

void appfw::BinaryReader::close() { m_File.close(); }

bool appfw::BinaryReader::isOpen() { return m_File.is_open(); }

size_t appfw::BinaryReader::getFileSize() {
    AFW_ASSERT_MSG(isOpen(), "Can't read a closed file");
    return m_iSize;
}

void appfw::BinaryReader::readBytes(char *data, size_t len) {
    AFW_ASSERT_MSG(isOpen(), "Can't read a closed file");
    m_File.read(data, len);

    if (m_File.eof()) {
        throw std::runtime_error("End of file reached.");
    }

    if (m_File.fail()) {
        throw std::runtime_error(fmt::format("Read error: {}", strerror(errno)));
    }
}
