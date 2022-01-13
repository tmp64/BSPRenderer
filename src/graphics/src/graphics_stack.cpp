#include <graphics/graphics_stack.h>
#include <graphics/gpu_buffer.h>

GraphicsStack::GraphicsStack() {
    
}

GraphicsStack::~GraphicsStack() {
    AFW_ASSERT_MSG(GPUBuffer::getGlobalMemUsage() == 0, "GPUBuffer leak");
}
