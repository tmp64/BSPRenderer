#include "material.h"
#include "filters.h"

namespace {

constexpr float TEXTURE_FILTER_WIDTH = 1;

float textureFilter(float x) {
    return rad::filters::linear1(x);
}

}

void rad::Material::loadProps(MaterialPropLoader &loader, std::string_view texName,
                              std::string_view wadName) {
    m_Props = loader.loadProperties(texName, wadName);
}

void rad::Material::loadFromRGB(RadSimImpl &radSim, int wide, int tall, const uint8_t *data) {
    AFW_ASSERT(wide > 0 && tall > 0);
    m_iWide = wide;
    m_iTall = tall;
    m_Pixels.resize(wide * tall);

    int stride = wide * 3;

    for (int i = 0; i < tall; i++) {
        for (int j = 0; j < wide; j++) {
            const uint8_t *bpx = data + stride * i + 3 * j;
            glm::vec3 fpx;
            fpx.r = bpx[0] / 255.0f;
            fpx.g = bpx[1] / 255.0f;
            fpx.b = bpx[2] / 255.0f;
            m_Pixels[wide * i + j] = radSim.gammaToLinear(fpx);
        }
    }
}

void rad::Material::loadFromRGBA(RadSimImpl &radSim, int wide, int tall, const uint8_t *data) {
    AFW_ASSERT(wide > 0 && tall > 0);
    m_iWide = wide;
    m_iTall = tall;
    m_Pixels.resize(wide * tall);

    int stride = wide * 4;

    for (int i = 0; i < tall; i++) {
        for (int j = 0; j < wide; j++) {
            const uint8_t *bpx = data + stride * i + 4 * j;
            glm::vec3 fpx;
            fpx.r = bpx[0] / 255.0f;
            fpx.g = bpx[1] / 255.0f;
            fpx.b = bpx[2] / 255.0f;
            m_Pixels[wide * i + j] = radSim.gammaToLinear(fpx);
        }
    }
}

void rad::Material::loadFromWad(RadSimImpl &radSim, const bsp::WADTexture &wadTex) {
    std::vector<uint8_t> data;

    if (wadTex.isTransparent()) {
        wadTex.getRGBA(data);
        loadFromRGBA(radSim, wadTex.getWide(), wadTex.getTall(), data.data());
    } else {
        wadTex.getRGB(data);
        loadFromRGB(radSim, wadTex.getWide(), wadTex.getTall(), data.data());
    }
}

glm::vec3 rad::Material::sampleColor(glm::vec2 pos, glm::vec2 scale) {
    glm::vec2 filterWidth = scale * TEXTURE_FILTER_WIDTH;
    int xfrom = (int)std::floor(pos.x - filterWidth.x) - 1;
    int xto = (int)std::ceil(pos.x + filterWidth.x) + 1;
    int yfrom = (int)std::floor(pos.y - filterWidth.y) - 1;
    int yto = (int)std::ceil(pos.y + filterWidth.y) + 1;

    glm::vec3 colorSum = glm::vec3(0, 0, 0);
    float weightSum = 0;
    glm::vec2 filterk = 1.0f / scale;

    for (int i = yfrom; i < yto; i++) {
        for (int j = xfrom; j < xto; j++) {
            glm::vec3 pixel = readColor(j, i);
            glm::vec2 curPos = glm::vec2(j, i) + glm::vec2(0.5f, 0.5f);
            glm::vec2 delta = curPos - pos;
            float filterx = textureFilter(delta.x * filterk.x);
            float filtery = textureFilter(delta.y * filterk.y);
            float weight = filterx * filtery;

            colorSum += pixel * weight;
            weightSum += weight;
        }
    }

    return colorSum / weightSum;
}
