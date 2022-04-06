#include <app_base/bitmap.h>
#include <graphics/texture2d_array.h>
#include <renderer/scene_shaders.h>
#include <hlviewer/assets/sprite_asset.h>
#include <hlviewer/assets/asset_manager.h>

static_assert(sizeof(glm::u8vec3) == 3, "glm::u8vec3 is not packed");
static_assert(sizeof(glm::u8vec4) == 4, "glm::u8vec4 is not packed");

void SpriteAsset::loadFromFile(AssetManager &assMgr, std::string_view path) {
    m_Path = path;
    bsp::Sprite sprite;
    sprite.loadFromFile(getFileSystem().findExistingFile(path));

    m_Model.spriteInfo = sprite.getInfo();
    glm::ivec2 spriteSize = {m_Model.spriteInfo.width, m_Model.spriteInfo.height};

    int framePixelCount = spriteSize.x * spriteSize.y;
    std::vector<glm::u8vec4> texdata(framePixelCount * m_Model.spriteInfo.numFrames);

    Bitmap<glm::u8vec4> tempBitmap;
    tempBitmap.init(spriteSize.x, spriteSize.y);

    // Load frames
    for (int i = 0; i < m_Model.spriteInfo.numFrames; i++) {
        const bsp::SpriteFrameInfo &frame = sprite.getFrameInfo(i);
        std::vector<glm::u8vec4> framePixels = decodeFrame(
            sprite.getPalette(), sprite.getFrameBitmap(i), m_Model.spriteInfo.texFormat);

        if (frame.size == spriteSize) {
            // Add to the texture as-is
            std::copy(framePixels.begin(), framePixels.end(),
                      texdata.begin() + framePixelCount * i);
        } else {
            // Expand to full frame
            tempBitmap.fill({0, 0, 0, 0});
            glm::ivec2 origin = glm::min(frame.origin, spriteSize) + spriteSize / 2; // Origin is set relative to center
            glm::ivec2 size = glm::min(frame.size, spriteSize - origin);
            tempBitmap.copyPixels(origin.x, origin.y, size.x, size.y, framePixels.data());
            std::copy(tempBitmap.getPixels().begin(), tempBitmap.getPixels().end(),
                      texdata.begin() + framePixelCount * i);
        }
    }

    // Upload to the GPU
    auto tex = std::make_unique<Texture2DArray>();
    tex->create(path);
    tex->initTexture(GraphicsFormat::RGBA8, spriteSize.x, spriteSize.y,
                     m_Model.spriteInfo.numFrames, true, GL_RGBA, GL_UNSIGNED_BYTE, texdata.data());
    tex->setWrapMode(TextureWrapMode::Clamp);

    // Create material
    m_Model.spriteMat = MaterialSystem::get().createMaterial(path);
    m_Model.spriteMat->setSize(1, 1);
    m_Model.spriteMat->setTexture(0, std::move(tex));
    m_Model.spriteMat->setShader(SHADER_TYPE_CUSTOM_IDX, &SceneShaders::Shaders::spriteShader);
    m_Model.spriteMat->setUsesGraphicalSettings(true);

    // Set up model
    m_Model.type = ModelType::Sprite;
    m_Model.flRadius = (float)std::max(spriteSize.x, spriteSize.y);

    assMgr.fakeDelay();
}

std::vector<glm::u8vec4> SpriteAsset::decodeFrame(const std::vector<uint8_t> &palette,
                                                  const std::vector<uint8_t> &bitmap,
                                                  bsp::SpriteFormat format) {
    const glm::u8vec3 *pal = reinterpret_cast<const glm::u8vec3 *>(palette.data());
    std::vector<glm::u8vec4> pixels(bitmap.size());

    for (size_t i = 0; i < pixels.size(); i++) {
        uint8_t palIdx = bitmap[i];
        glm::u8vec3 color = pal[palIdx];

        switch (format) {
        case bsp::SPR_NORMAL:
        case bsp::SPR_ADDITIVE: {
            pixels[i] = glm::u8vec4(color, 255);
            break;
        }
        case bsp::SPR_INDEXALPHA: {
            pixels[i] = glm::u8vec4(color, palIdx);
            break;
        }
        case bsp::SPR_ALPHATEST: {
            if (palIdx == 255) {
                pixels[i] = glm::u8vec4(0, 0, 0, 0);
            } else {
                pixels[i] = glm::u8vec4(color, 255);
            }
            break;
        }
        }
    }

    return pixels;
}
