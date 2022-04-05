#include <appfw/binary_buffer.h>
#include <bsp/sprite.h>

void bsp::Sprite::loadFromFile(const fs::path &path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (file.fail()) {
        throw std::runtime_error(std::string("Failed to open file: ") + strerror(errno));
    }

    std::vector<uint8_t> data;
    appfw::readFileContents(file, data);
    loadFromBuffer(appfw::span(data));
}

void bsp::Sprite::loadFromBuffer(appfw::span<const uint8_t> data) {
    appfw::BinaryBuffer buf(data.remove_const());

    uint32_t magic = buf.readUInt32();
    if (magic != SPR_MAGIC) {
        throw std::runtime_error(
            fmt::format("invalid magic {:X} (should be {:X})", magic, SPR_MAGIC));
    }

    uint32_t version = buf.readUInt32();
    if (version != SPR_VERSION) {
        throw std::runtime_error(
            fmt::format("invalid version {} (should be {})", version, SPR_VERSION));
    }

    m_Info = readInfo(buf);

    if (m_Info.numFrames < 1) {
        throw std::runtime_error(fmt::format("invalid frame number ({})", m_Info.numFrames));
    }

    m_Palette = readPalette(buf);
    m_Frames.resize(m_Info.numFrames);
    m_Bitmaps.resize(m_Info.numFrames);

    for (int i = 0; i < m_Info.numFrames; i++) {
        SpriteFrameType type = (SpriteFrameType)buf.readInt32();
        if (type == SPR_FRAME_NORMAL) {
            readSpriteFrame(buf, m_Frames[i], m_Bitmaps[i]);
        } else if (type == SPR_FRAME_GROUP) {
            // I don't think GoldSrc supports them either.
            // It can load them (the code is there) but R_GetSpriteFrame returns nullptr
            // if frame is a group.
            throw std::runtime_error("Sprite frame groups are not supported");
        } else {
            throw std::runtime_error(fmt::format("frame #{}: invalid type {}", i, (int)type));
        }
    }
}

bsp::SpriteInfo bsp::Sprite::readInfo(appfw::BinaryInputStream &buf) {
    SpriteInfo info;
    info.type = (SpriteType)buf.readInt32();
    info.texFormat = (SpriteFormat)buf.readInt32();
    info.boundingRadius = buf.readFloat();
    info.width = buf.readInt32();
    info.height = buf.readInt32();
    info.numFrames = buf.readInt32();
    info.beamLength = buf.readFloat();
    info.syncType = (SpriteSyncType)buf.readInt32();
    return info;
}

std::vector<uint8_t> bsp::Sprite::readPalette(appfw::BinaryInputStream &buf) {
    uint16_t colorCount = buf.readUInt16();
    if (colorCount != 256) {
        throw std::runtime_error(fmt::format("invalid color count ({})", colorCount));
    }

    std::vector<uint8_t> palette(colorCount * 3);
    buf.readBytes(palette.data(), palette.size());
    return palette;
}

void bsp::Sprite::readSpriteFrame(appfw::BinaryInputStream &buf, SpriteFrameInfo &info,
                                  std::vector<uint8_t> &bitmap) {
    info.origin.x = buf.readInt32();
    info.origin.y = buf.readInt32();
    info.size.x = buf.readInt32();
    info.size.y = buf.readInt32();

    bitmap.resize(info.size.x * info.size.y);
    buf.readBytes(bitmap.data(), bitmap.size());
}
