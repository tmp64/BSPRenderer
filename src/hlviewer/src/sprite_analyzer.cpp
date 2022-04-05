#include <bsp/sprite.h>
#include "sprite_analyzer.h"

namespace {

const char *s_SpriteTypes[] = {
    "VP_PARALLEL_UPRIGHT", "FACING_UPRIGHT", "VP_PARALLEL", "ORIENTED", "VP_PARALLEL_ORIENTED",
};

const char *s_SpriteFormats[] = {
    "SPR_NORMAL",
    "SPR_ADDITIVE",
    "SPR_INDEXALPHA",
    "SPR_ALPHTEST",
};

const char *s_SyncTypes[] = {
    "ST_SYNC",
    "ST_RAND",
};

void recursiveFindSprites(std::set<std::string> &files, std::string path) {
    auto allFiles = getFileSystem().getDirEntries("assets:sprites" + path);

    for (const auto &[name, dirEntry] : allFiles) {
        if (dirEntry.is_directory()) {
            recursiveFindSprites(files, fmt::format("{}/{}", path, name));
        } else {
            std::string_view nameView = name;
            if (nameView.size() >= 4 && nameView.substr(nameView.size() - 4, 4) == ".spr") {
                files.insert(fmt::format("{}/{}", path, name));
            }
        }
    }
}

} // namespace

SpriteAnalyzer::SpriteAnalyzer() {
    setTitle("Sprite Analyzer");
    setDefaultSize({640, 480});

    appfw::Timer timer;
    printn("ANALYZING ALL SPRITES");

    // Find all sprites
    std::set<std::string> files;
    recursiveFindSprites(files, "");
    m_FileCount = (int)files.size();
    printi("Found sprites: {}", m_FileCount);

    // Open them
    for (auto &filename : files) {
        bsp::Sprite sprite;

        try {
            std::string path = fmt::format("assets:sprites/{}", filename);
            sprite.loadFromFile(getFileSystem().findExistingFile(path));
            m_OpenCount++;
        } catch (const std::exception &e) {
            printe("Sprite {} failed to load: {}", filename, e.what());
            continue;
        }

        analyzeSprite(filename, sprite);
    }

    printn("{} processed, {} failed", m_OpenCount, m_FileCount - m_OpenCount);
    printn("Time: {:.3f} s", timer.dseconds());
}

void SpriteAnalyzer::showContents() {
    ImGui::Text("Total files: %d", m_FileCount);
    ImGui::Text("Processed: %d (errors: %d)", m_OpenCount, m_FileCount - m_OpenCount);

    if (ImGui::BeginTabBar("TabBar")) {
        if (ImGui::BeginTabItem("By Type")) {
            showBy(std::size(m_ByType), s_SpriteTypes, m_ByType);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("By Format")) {
            showBy(std::size(m_ByFormat), s_SpriteFormats, m_ByFormat);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("By Sync Type")) {
            showBy(std::size(m_BySyncType), s_SyncTypes, m_BySyncType);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Limits")) {
            ImGui::Text("  Min size: %3d x %3d", m_MinSize.x, m_MinSize.y);
            ImGui::Text("  Max size: %3d x %3d", m_MaxSize.x, m_MaxSize.y);
            ImGui::Text("Min frames: %d", m_MinFrames);
            ImGui::Text("Max frames: %d", m_MaxFrames);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void SpriteAnalyzer::analyzeSprite(const std::string &name, const bsp::Sprite &sprite) {
    const bsp::SpriteInfo &info = sprite.getInfo();

    // Type
    int type = (int)info.type;
    if (type >= 0 && type < std::size(m_ByType)) {
        m_ByType[type]++;
    } else {
        printw("{}: invalid type {}", name, type);
    }

    // Format
    int format = (int)info.texFormat;
    if (format >= 0 && format < std::size(m_ByFormat)) {
        m_ByFormat[format]++;
    } else {
        printw("{}: invalid format {}", name, format);
    }

    // Sync type
    int syncType = (int)info.syncType;
    if (syncType >= 0 && syncType < std::size(m_BySyncType)) {
        m_BySyncType[syncType]++;
    } else {
        printw("{}: invalid sync type {}", name, syncType);
    }

    m_MinSize = glm::min(m_MinSize, glm::ivec2(info.width, info.height));
    m_MaxSize = glm::max(m_MaxSize, glm::ivec2(info.width, info.height));

    m_MinFrames = std::min(m_MinFrames, info.numFrames);
    m_MaxFrames = std::max(m_MaxFrames, info.numFrames);
}

void SpriteAnalyzer::showBy(size_t itemCount, const char *lables[], int values[]) {
    for (size_t i = 0; i < itemCount; i++) {
        ImGui::Text("%20s: %d", lables[i], values[i]);
    }
}
