#include <appfw/str_utils.h>
#include <hlviewer/assets/wad_asset.h>
#include <hlviewer/assets/asset_manager.h>

bool WADAsset::nameEquals(std::string_view name) {
    if (m_WADName.size() != name.size()) {
        return false;
    }

    return appfw::strncasecmp(name.data(), m_WADName.c_str(), name.size()) == 0;
}

int WADAsset::indexOfTexture(const char *name) const {
    auto &textures = m_File.getTextures();
    for (int i = 0; i < textures.size(); i++) {
        if (!appfw::strcasecmp(textures[i].getName(), name)) {
            return i;
        }
    }

    return -1;
}

const WADMaterialAsset *WADAsset::loadMaterial(int index) {
    AFW_ASSERT(index > 0 && index < (int)m_Materials.size());

    // See if it's already loaded
    if (m_Materials[index]) {
        return m_Materials[index].get();
    }

    // Find the texture
    const bsp::WADTexture &tex = m_File.getTextures()[index];

    // Load texture data
    bool isRgba = tex.isTransparent();
    std::vector<uint8_t> data;

    if (isRgba) {
        tex.getRGBA(data);
    } else {
        tex.getRGB(data);
    }

    // Create the asset
    m_Materials[index] = std::make_unique<WADMaterialAsset>();
    WADMaterialAsset *mat = m_Materials[index].get();
    mat->init(tex.getName(), m_WADName, tex.getWide(), tex.getTall(), isRgba, data);

    AssetManager::get().fakeDelay();
    return mat;
}

std::string WADAsset::extractNameFromPath(std::string_view path) {
    size_t pos = path.find_first_of(':');
    AFW_ASSERT(pos != path.npos);
    std::string name = std::string(path.substr(pos + 1));
    appfw::strToLower(name.begin(), name.end());
    return name;
}


const bsp::WADTexture *WADAsset::findTexture(const char *name) {
    auto &list = m_File.getTextures();
    auto it = std::find_if(list.begin(), list.end(), [name](const bsp::WADTexture &tex) {
        return !appfw::strcasecmp(tex.getName(), name);
    });

    if (it == list.end()) {
        return nullptr;
    } else {
        return &(*it);
    }
}

void WADAsset::loadFromFile(AssetManager &assMgr, std::string_view path, std::string_view wadName) {
    m_Path = path;
    m_File.loadFromFile(getFileSystem().findExistingFile(path));
    m_WADName = wadName;
    m_Materials.resize(m_File.getTextures().size());

    assMgr.fakeDelay();
}
