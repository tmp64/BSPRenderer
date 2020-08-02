#ifndef RENDERER_FRAME_CONSOLE_H
#define RENDERER_FRAME_CONSOLE_H
#include <appfw/utils.h>
#include <renderer/font_renderer.h>

/**
 * A console that draws directly to the framebuffer.
 * Any written data is only valid for one frame.
 */
class FrameConsole : appfw::utils::NoCopy {
public:
    static constexpr char FONT_NAME[] = "fonts/courbd.ttf";
    static constexpr int FONT_SIZE = 20;
    static constexpr float LINE_HEIGHT = 18;
    static constexpr float TOP_OFFSET = 10;
    static constexpr float SIDE_OFFSET = 20;

    inline static appfw::Color Red = appfw::Color(222, 51, 51, 255);
    inline static appfw::Color Green = appfw::Color(85, 222, 22, 255);
    inline static appfw::Color Blue = appfw::Color(30, 30, 227, 255);
    inline static appfw::Color Cyan = appfw::Color(30, 220, 227, 255);
    inline static appfw::Color White = appfw::Color(255, 255, 255, 255);
    
    FrameConsole();

    /**
     * Resets internal state. Must be called after screen was cleared.
     */
    void reset();

    /**
     * Print a line on the left part of the screen.
     */
    void printLeft(appfw::Color color, const std::string &msg);

    /**
     * Print a line on the right part of the screen.
     */
    void printRight(appfw::Color color, const std::string &msg);

    /**
     * Must be called when viewport size has changed.
     * @param   wide    New width of the viewport
     * @param   tall    New height of the viewport
     */
    void updateViewportSize(int wide, int tall);

private:
    FontRenderer m_FontRenderer;
    int m_iWide = 0, m_iTall = 0;
    float m_flLeftX = 0, m_flLeftY = 0;
    float m_flRightX = 0, m_flRightY = 0;
};

#endif
