#include <stb_image.h>
#include <renderer/envmap.h>

static std::vector<uint8_t> rotateImage90CW(uint8_t *data, int wide, int tall) {
    std::vector<uint8_t> newData(3 * wide * tall);

    auto fnGetPixelPtr = [](uint8_t *data, int wide, [[maybe_unused]] int tall, int col, int row) {
        return data + row * wide * 3 + col * 3;
    };

    for (int col = 0; col < wide; col++) {
        for (int row = 0; row < tall; row++) {
            uint8_t *pold = fnGetPixelPtr(data, wide, tall, col, tall - row - 1);
            uint8_t *pnew = fnGetPixelPtr(newData.data(), tall, wide, row, col);
            memcpy(pnew, pold, 3);
        }
    }

    return newData;
}

static std::vector<uint8_t> rotateImage90CCW(uint8_t *data, int wide, int tall) {
    std::vector<uint8_t> newData(3 * wide * tall);

    auto fnGetPixelPtr = [](uint8_t *data, int wide, [[maybe_unused]] int tall, int col, int row) {
        return data + row * wide * 3 + col * 3;
    };

    for (int col = 0; col < wide; col++) {
        for (int row = 0; row < tall; row++) {
            uint8_t *pold = fnGetPixelPtr(data, wide, tall, wide - col - 1, row);
            uint8_t *pnew = fnGetPixelPtr(newData.data(), tall, wide, row, col);
            memcpy(pnew, pold, 3);
        }
    }

    return newData;
}

TextureCube loadEnvMapImage(std::string_view textureName, std::string_view vpath) {
    // Filename suffixes for sides
    constexpr const char *suffixes[] = {"rt", "lf", "up", "dn", "bk", "ft"};

    // Detect TGA or BMP
    bool isTga = false;
    {
        // Try TGA first
        std::string curvpath = fmt::format("{}{}.tga", vpath, suffixes[0]);
        fs::path curpath = getFileSystem().findExistingFile(curvpath, std::nothrow);

        if (curpath.empty()) {
            // Try BMP instead
            curvpath = fmt::format("{}{}.bmp", vpath, suffixes[0]);
            curpath = getFileSystem().findExistingFile(curvpath, std::nothrow);
            isTga = false;
        } else {
            isTga = true;
        }

        if (curpath.empty()) {
            throw std::runtime_error(fmt::format(
                "failed to locate either TGA or BMP version of '{}{}'", vpath, suffixes[0]));
        }
    }

    std::vector<uint8_t> sidesData[6];
    int cubeSize = 0;

    for (int i = 0; i < 6; i++) {
        std::string curvpath;
        fs::path curpath;

        if (isTga) {
            curvpath = fmt::format("{}{}.tga", vpath, suffixes[i]);
            curpath = getFileSystem().findExistingFile(curvpath);
        } else {
            curvpath = fmt::format("{}{}.bmp", vpath, suffixes[i]);
            curpath = getFileSystem().findExistingFile(curvpath);
        }

        int width, height, channelNum;
        std::unique_ptr<uint8_t, decltype(&stbi_image_free)> data(
            stbi_load(curpath.u8string().c_str(), &width, &height, &channelNum, 3),
            &stbi_image_free);

        if (!data) {
            throw std::runtime_error(
                fmt::format("{}: failed to load: {}", curvpath, stbi_failure_reason()));
        }

        // Validate size
        if (width != height) {
            throw std::runtime_error(fmt::format("{}: image is not square", curvpath));
        } else if (cubeSize == 0) {
            // Set the cubemap size
            cubeSize = width;
        } else if (cubeSize != width) {
            throw std::runtime_error(
                fmt::format("{}: image size is different ({} vs {})", curvpath, width, cubeSize));
        }

        // Save the data
        if (i == 2) {
            // Top sky texture needs to be rotated CCW
            sidesData[i] = rotateImage90CCW(data.get(), width, height);
        } else if (i == 3) {
            // Bottom sky texture needs to be rotated CW
            sidesData[i] = rotateImage90CW(data.get(), width, height);
        } else {
            sidesData[i].resize(width * height * 3);
            std::copy(data.get(), data.get() + sidesData[i].size(), sidesData[i].begin());
        }
    }

    // Create texture
    const void *datas[6];

    for (int i = 0; i < 6; i++) {
        datas[i] = sidesData[i].data();
    }

    TextureCube texture;
    texture.create(textureName);
    texture.initTexture(GraphicsFormat::RGB8, cubeSize, cubeSize, true, GL_RGB, GL_UNSIGNED_BYTE,
                        datas);
    texture.setWrapMode(TextureWrapMode::Clamp);
    texture.setFilter(TextureFilter::Trilinear);
    return texture;
}

TextureCube createErrorEnvMap(std::string_view textureName) {
    int size = CheckerboardImage::get().size;
    const void *data = CheckerboardImage::get().data.data();
    const void *datas[6];
    std::fill(datas, datas + 6, data);

    TextureCube texture;
    texture.create(textureName);
    texture.initTexture(GraphicsFormat::RGB8, size, size, true, GL_RGB, GL_UNSIGNED_BYTE, datas);
    texture.setWrapMode(TextureWrapMode::Clamp);
    return texture;
}
