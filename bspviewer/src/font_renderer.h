#ifndef FONT_RENDERER_H
#define FONT_RENDERER_H

namespace fontrndr
{
	void Initialize();
	void Shutdown();
	int DrawChar(int x, int y, wchar_t c, float scale = 1.0f);
}

class FontRenderer {
public:

private:

};

#endif
