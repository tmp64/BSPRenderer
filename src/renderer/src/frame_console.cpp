#include <renderer/frame_console.h>

FrameConsole::FrameConsole() : m_FontRenderer(FONT_NAME, FONT_SIZE) {}

void FrameConsole::reset() {
	m_flLeftX = SIDE_OFFSET;
    m_flLeftY = TOP_OFFSET + LINE_HEIGHT;

	m_flRightX = m_iWide - SIDE_OFFSET;
    m_flRightY = TOP_OFFSET;
}

void FrameConsole::printLeft(appfw::Color color, const std::string &msg) {
    m_FontRenderer.setFontColor(color);
    for (char c : msg) {
        if (c == '\n') {
            m_flLeftX = SIDE_OFFSET;
            m_flLeftY += LINE_HEIGHT;
        } else {
            m_flLeftX += m_FontRenderer.drawChar(m_flLeftX, m_flLeftY, c);
        }
    }
}

void FrameConsole::printRight(appfw::Color color, const std::string &msg) {
    m_FontRenderer.setFontColor(color);
    for (auto it = msg.rbegin(); it != msg.rend(); ++it) {
        char c = *it;

        if (c == '\n') {
            m_flRightX = m_iWide - SIDE_OFFSET;
            m_flRightY += LINE_HEIGHT;
        } else {
            m_flRightX -= m_FontRenderer.getCharWidth(c);
            m_FontRenderer.drawChar(m_flRightX, m_flRightY, c);
        }
    }
}

void FrameConsole::updateViewportSize(int wide, int tall) {
    m_FontRenderer.updateViewportSize(wide, tall);
	m_iWide = wide;
    m_iTall = tall;
}
