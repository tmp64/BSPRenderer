#ifndef APP_BASE_LIGHTMAP_H
#define APP_BASE_LIGHTMAP_H

struct LightmapFileFormat {
    static constexpr uint8_t MAGIC[] = "LM002";

    enum class Format : uint8_t
    {
        Unknown = 0,
        RGBF32 = 1,
    };

    enum class Compression : uint8_t
    {
        None = 0,
    };
};

#endif
