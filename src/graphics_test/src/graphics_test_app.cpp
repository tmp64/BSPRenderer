#include "graphics_test_app.h"

const AppInitInfo &app_getInitInfo() {
    static AppInitInfo info = {"graphics_test", fs::u8path("")};
    return info;
}

std::unique_ptr<AppBase> app_createSingleton() {
    return std::make_unique<GraphicsTestApp>();
}

void GraphicsTestApp::lateInit() {
    testGPUBuffer();
}

void GraphicsTestApp::testGPUBuffer() {
    // Test buffer creation
    GPUBuffer buffer;
    buffer.create(GL_ARRAY_BUFFER, "Test");
    buffer.init(128, nullptr, GL_DYNAMIC_DRAW);
    AFW_ASSERT(buffer.getMemUsage() == 128);
    AFW_ASSERT(GPUBuffer::getGlobalMemUsage() == 128);

    // Test buffer move
    GPUBuffer buffer2 = std::move(buffer);
    AFW_ASSERT(buffer.getMemUsage() == 0);
    AFW_ASSERT(buffer2.getMemUsage() == 128);
    AFW_ASSERT(GPUBuffer::getGlobalMemUsage() == 128);

    // Add more buffers
    GPUBuffer buffer3, buffer4;
    buffer3.create(GL_ARRAY_BUFFER, "Test 3");
    buffer3.init(1000, nullptr, GL_DYNAMIC_DRAW);
    buffer4.create(GL_ARRAY_BUFFER, "Test 4");
    buffer4.init(2000, nullptr, GL_DYNAMIC_DRAW);
    AFW_ASSERT(GPUBuffer::getGlobalMemUsage() == 3128);

    buffer3.destroy();
    AFW_ASSERT(GPUBuffer::getGlobalMemUsage() == 2128);

    buffer4.destroy();
    AFW_ASSERT(GPUBuffer::getGlobalMemUsage() == 128);
}
