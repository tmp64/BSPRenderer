#include <gui_app_base/gui_app_base.h>
#include "profiler_dialog.h"

ConVar<bool> prof_ui("prof_ui", true, "Show profiler dialog");

void ProfilerDialog::tick() {
    appfw::Prof prof("Profiler Dialog");
    bool show = prof_ui.getValue();

    if (!show) {
        return;
    }

    graphTick();

    ImGui::SetNextWindowBgAlpha(0.2f);

    if (!ImGui::Begin("Profiler", &show)) {
        ImGui::End();
        return;
    }

    if (!ImGui::BeginTabBar("ProfilerTabBar", ImGuiTabBarFlags_None)) {
        ImGui::End();
        return;
    }

    auto proflist = appfw::ProfData::getDataList();

    for (appfw::ProfData *profData : proflist) {
        if (ImGui::BeginTabItem(profData->getName())) {
            m_pSelectedData = profData; 
            const appfw::ProfNode &root = profData->getPrevRootNode();

            if (root.pSection) {
                float width = ImGui::GetWindowContentRegionWidth() - 6.0f * ImGui::GetFontSize();
                ImGui::Columns(2, "tree", true);
                ImGui::SetColumnWidth(0, width);
                showNode(root);
            }

            ImGui::EndTabItem();
        }
    }

    ImGui::EndTabBar();
    ImGui::End();
    if (show != prof_ui.getValue()) {
        prof_ui.setValue(show);
    }
}

void ProfilerDialog::showNode(const appfw::ProfNode &node) {
    bool hasChildren = !node.children.empty();
    bool isOpen = false;

    if (hasChildren) {
        int flags = ImGuiTreeNodeFlags_DefaultOpen;
        isOpen = ImGui::TreeNodeEx((void *)node.pSection, flags, "%s", node.pSection->name);
    } else {
        ImGui::Indent();
        ImGui::Text("%s", node.pSection->name);
        ImGui::Unindent();
    }

    ImGui::NextColumn();
    size_t hash = node.pSection->uHash;
    std::string time = fmt::format("{:.3f} ms###{}", node.pSection->flTime[1] * 1000, hash);
    if (ImGui::Selectable(time.c_str(), hash == m_uSelectedSection,
                          ImGuiSelectableFlags_SpanAllColumns)) {
        m_uSelectedSection = hash;
        resetGraphData();
    }
    ImGui::NextColumn();

    if (isOpen) {
        double timeSum = 0;

        for (const appfw::ProfNode &i : node.children) {
            showNode(i);
            timeSum += i.pSection->flTime[1];
        }

        double timeLost = node.pSection->flTime[1] - timeSum;
        if (!node.children.empty() && timeLost > appfw::ProfData::getMinLostTime()) {
            ImGui::Indent();
            ImGui::Text("(time lost)");
            ImGui::Unindent();
            ImGui::NextColumn();
            ImGui::Text("%.3f ms", timeLost * 1000);
            ImGui::NextColumn();
        }

        ImGui::TreePop();
    }
}

void ProfilerDialog::graphTick() {
    if (m_uSelectedSection == 0) {
        return;
    }

    if (!m_pSelectedData) {
        m_uSelectedSection = 0;
        return;
    }   

    appfw::ProfSection *section = m_pSelectedData->getSectionByHash(m_uSelectedSection);

    if (!section) {
        m_uSelectedSection = 0;
        return;
    }   

    // Record time
    m_TimingLog[m_uTimingIndex] = (float)section->flTime[0] * 1000.0f;
    m_uTimingIndex++;

    if (m_uTimingIndex == m_TimingLog.size()) {
        m_uTimingIndex = 0;
    }

    bool isOpen = true;
    ImGui::SetNextWindowBgAlpha(0.2f);

    if (!ImGui::Begin("Timing Graph", &isOpen)) {
        ImGui::End();
        return;
    }

    ImVec2 min = ImGui::GetWindowContentRegionMin();
    ImVec2 max = ImGui::GetWindowContentRegionMax();
    ImVec2 size = ImVec2(max.x - min.x, max.y - min.y);
    ImGui::PlotLines("Time", plotGetter, this, (int)m_TimingLog.size(), 0, nullptr, 0, FLT_MAX,
                     size);

    ImGui::End();

    if (!isOpen) {
        m_uSelectedSection = 0;
        resetGraphData();
    }
}

void ProfilerDialog::resetGraphData() {
    m_TimingLog.clear();
    m_uTimingIndex = 0;

    if (m_uSelectedSection != 0) {
        m_TimingLog.resize((size_t)(m_flRecordSecs * GuiAppBase::getBaseInstance().getFrameRate()));
    }
}

float ProfilerDialog::plotGetter(void *data, int idxx) {
    auto *d = static_cast<ProfilerDialog *>(data);
    size_t idx = idxx;
    size_t a = d->m_TimingLog.size() - d->m_uTimingIndex;

    if (idx < a) {
        // Elements including and after m_uTimingIndex
        return d->m_TimingLog[d->m_uTimingIndex + idx];
    } else {
        // Elements before m_uTimingIndex
        return d->m_TimingLog[idx - a];
    }
}
