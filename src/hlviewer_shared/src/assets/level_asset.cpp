#include <bsp/utils.h>
#include <bsp/entity_key_values.h>
#include <hlviewer/assets/asset_manager.h>
#include <hlviewer/assets/level_asset.h>

std::list<std::pair<WADAsset *, int>> LevelAsset::findTexturesToLoad() {
    std::list<std::pair<WADAsset *, int>> list;

    for (const bsp::BSPMipTex &tex : m_Level.getTextures()) {
        for (WADAsset *wad : m_Wads) {
            int texIdx = wad->indexOfTexture(tex.szName);
            if (texIdx != -1 && !wad->getMaterial(texIdx)) {
                list.push_back({wad, texIdx});
            }
        }
    }

    return list;
}

Material *LevelAsset::findMaterial(const char *name) {
    for (WADAsset *wad : m_Wads) {
        int texIdx = wad->indexOfTexture(name);
        if (texIdx != -1) {
            if (const WADMaterialAsset *mat = wad->getMaterial(texIdx)) {
                return mat->getMaterial();
            }
        }
    }

    return nullptr;
}

void LevelAsset::loadFromFile(AssetManager &assMgr, std::string_view path) {
    m_Path = path;
    m_Level.loadFromFile(getFileSystem().findExistingFile(path));

    // Load all WADs
    std::vector<std::string> requiredWads = getRequiredWads();
    m_Wads.reserve(requiredWads.size());

    for (const std::string &reqWad : requiredWads) {
        try {
            m_Wads.push_back(&assMgr.loadWad("assets:" + reqWad));
        } catch (const std::exception &e) {
            printe("WAD file '{}' failed to load: {}", reqWad, e.what());
        }
    }

    assMgr.fakeDelay();
}

std::vector<std::string> LevelAsset::getRequiredWads() {
    bsp::EntityKeyValuesDict entities(m_Level.getEntitiesLump());
    const bsp::EntityKeyValues &worldspawn = entities[0];
    int wadIdx = worldspawn.indexOf("wad");
    std::string wads = wadIdx != -1 ? worldspawn.get(wadIdx).asString() : "";

    if (wads.empty()) {
        printw("Level '{}' doesn't reference any WAD files.", m_Path);
        return {};
    }

    return bsp::parseWadListString(wads);
}
