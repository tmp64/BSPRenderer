#ifndef BSP_SPRITE_H
#define BSP_SPRITE_H
#include <vector>
#include <appfw/span.h>
#include <bsp/bsp_types.h>

// Based on Unofficial Half-Life WAD3 and SPRITE file format specification
// https://yuraj.ucoz.com/half-life-formats.pdf

namespace bsp {

constexpr uint32_t SPR_MAGIC = 'I' | ('D' << 8) | ('S' << 16) | ('P' << 24);
constexpr uint32_t SPR_VERSION = 2;

enum SpriteType
{
    SPR_VP_PARALLEL_UPRIGHT = 0,
    SPR_FACING_UPRIGHT,
    SPR_VP_PARALLEL,
    SPR_ORIENTED,
    SPR_VP_PARALLEL_ORIENTED,
};

enum SpriteFormat
{
    SPR_NORMAL = 0,
    SPR_ADDITIVE,
    SPR_INDEXALPHA,
    SPR_ALPHATEST,
};

enum SpriteSyncType
{
    ST_SYNC = 0,
    ST_RAND,
};

enum SpriteFrameType
{
    SPR_FRAME_NORMAL = 0,
    SPR_FRAME_GROUP
};

struct SpriteInfo {
    SpriteType type;
    SpriteFormat texFormat;
    float boundingRadius;
    int width;
    int height;
    int numFrames;
    float beamLength;
    SpriteSyncType syncType;
};

struct SpriteFrameInfo {
    glm::ivec2 origin;
    glm::ivec2 size;
};

class Sprite {
public:
    //! Loads the sprite from a file.
    void loadFromFile(const fs::path &path);

    //! Loads the sprite from a buffer.
    void loadFromBuffer(appfw::span<const uint8_t> data);

    //! @returns the sprite header.
    inline const SpriteInfo &getInfo() const { return m_Info; }

    //! @returns the palette
    inline const std::vector<uint8_t> &getPalette() { return m_Palette; }

    //! @returns the frame info
    inline const SpriteFrameInfo &getFrameInfo(int frame) { return m_Frames[frame]; }

    //! @returns the frame bitmap
    inline const std::vector<uint8_t> &getFrameBitmap(int frame) { return m_Bitmaps[frame]; }

private:
    SpriteInfo m_Info;
    std::vector<uint8_t> m_Palette;
    std::vector<SpriteFrameInfo> m_Frames;
    std::vector<std::vector<uint8_t>> m_Bitmaps;

    SpriteInfo readInfo(appfw::BinaryInputStream &buf);
    std::vector<uint8_t> readPalette(appfw::BinaryInputStream &buf);
    void readSpriteFrame(appfw::BinaryInputStream &buf, SpriteFrameInfo &info,
                         std::vector<uint8_t> &bitmap);
};

} // namespace bsp

#endif
