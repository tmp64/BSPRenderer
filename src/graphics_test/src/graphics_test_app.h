#ifndef GRAPHICS_TEST_APP_H
#define GRAPHICS_TEST_APP_H
#include <graphics/graphics_stack.h>
#include <graphics/gpu_buffer.h>
#include <gui_app_base/gui_app_base.h>

class GraphicsTestApp : public GuiAppBase {
public:
    void lateInit() override;

private:
    void testGPUBuffer();
};

#endif
