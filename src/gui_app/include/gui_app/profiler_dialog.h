#ifndef GUI_APP_PROFILER_DIALOG_H
#define GUI_APP_PROFILER_DIALOG_H
#include <vector>
#include <imgui.h>
#include <appfw/prof.h>

class ProfilerDialog : appfw::NoMove {
public:
    void tick();

private:
    appfw::ProfData *m_pSelectedData = nullptr;
    size_t m_uSelectedSection = 0;

    // Graph
    float m_flRecordSecs = 5.0f;
    std::vector<float> m_TimingLog;
    size_t m_uTimingIndex = 0;

    void showNode(const appfw::ProfNode &node);
    void graphTick();
    void resetGraphData();

    static float plotGetter(void *data, int idxx);
};

#endif
