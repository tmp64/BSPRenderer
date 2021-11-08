#include <appfw/str_utils.h>
#include "wad_asset.h"
#include "asset_manager.h"

bool WADAsset::nameEquals(std::string_view name) {
    if (m_WADName.size() != name.size()) {
        return false;
    }

    // caseless compare
    for (size_t i = 0; i < name.size(); i++) {
        if (std::tolower(name[i]) != std::tolower(m_WADName[i])) {
            return false;
        }
    }

    return true;
}

WADMaterialAssetRef WADAsset::findLoadedMaterial(const char *name) const {
    std::shared_lock lock(m_Mutex);
    return findLoadedMaterialInternal(name);
}

std::pair<WADMaterialAssetRef, WADMaterialAsset::UploadTask>
WADAsset::loadMaterialInWorker(const char *name) {
    std::unique_lock lock(m_Mutex);

    // See if it's already loaded
    WADMaterialAssetRef mat = findLoadedMaterialInternal(name);
    if (mat) {
        return {mat, WADMaterialAsset::UploadTask()};
    }

    // Find the texture
    const bsp::WADTexture *tex = findTexture(name);
    if (!tex) {
        return {WADMaterialAssetRef(), WADMaterialAsset::UploadTask()};
    }

    // Create a new asset
    mat.set(&(*m_Materials.emplace(m_Materials.end())));
    
    // Load texture data
    bool isRgba = tex->isTransparent();
    std::vector<uint8_t> data;
    
    if (isRgba) {
        tex->getRGBA(data);
    } else {
        tex->getRGB(data);
    }

    auto task = mat->init(tex->getName(), m_WADName, tex->getWide(), tex->getTall(), isRgba, std::move(data));
    return {mat, std::move(task)};
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

WADMaterialAssetRef WADAsset::findLoadedMaterialInternal(const char *name) const {
    for (const WADMaterialAsset &mat : m_Materials) {
        if (mat.m_bIsFullyLoaded && mat.getMaterial()->getName() == name) {
            return WADMaterialAssetRef(const_cast<WADMaterialAsset *>(&mat));
        }
    }

    return WADMaterialAssetRef();
}

void WADAsset::loadFromFile(AssetManager &assMgr, std::string_view path) {
    m_Path = path;
    m_File.loadFromFile(getFileSystem().findExistingFile(path));
    size_t pos = path.find_first_of(':');
    AFW_ASSERT(pos != path.npos);
    m_WADName = path.substr(pos + 1);
    appfw::strToLower(m_WADName.begin(), m_WADName.end());

    assMgr.fakeDelay();

    m_bIsFullyLoaded = true;
    m_bIsLoading = false;
}
