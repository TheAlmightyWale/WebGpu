#include "GraphicsContext.h"
#include <glfw3webgpu.h>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <cassert>

#include "Quad.h"
#include "QuadDefs.h"
#include "Core/ResourceDefs.h"

namespace
{
    constexpr uint32_t k_kbBytes = 1024;
    constexpr uint32_t k_mbBytes = 1024 * 1024;
}

namespace Gfx
{

GraphicsContext::GraphicsContext(Window &window)
    : _instance(nullptr), _surface(nullptr), _adapter(nullptr), _device(nullptr), _queue(nullptr)
{
    InitializeInstance();
    CreateSurface(window);
    RequestAdapter();
    LogAdapterFeatures();
    ConfigureDeviceLimits();
    CreateDevice();
    SetupErrorHandling();
}

GraphicsContext::~GraphicsContext()
{
    // Release in reverse order of creation
    _queue.release();
    _device.release();
    _adapter.release();
    _surface.release();
    _instance.release();
}

void GraphicsContext::InitializeInstance()
{
    wgpu::InstanceDescriptor desc{};
    _instance = wgpu::createInstance(desc);

    if (!_instance)
    {
        throw std::runtime_error("Could not initialize WebGPU instance");
    }

    std::cout << "WGPU Instance: " << _instance << "\n";
}

void GraphicsContext::CreateSurface(Window &window)
{
    _surface = wgpu::Surface{glfwGetWGPUSurface(_instance, window.get())};

    if (!_surface)
    {
        throw std::runtime_error("Could not create WebGPU surface");
    }
}

void GraphicsContext::RequestAdapter()
{
    wgpu::RequestAdapterOptions adapterOptions{};
    adapterOptions.compatibleSurface = _surface;
    _adapter = _instance.requestAdapter(adapterOptions);

    if (!_adapter)
    {
        throw std::runtime_error("Could not request WebGPU adapter");
    }

    _adapter.getLimits(&_adapterLimits);
    std::cout << "adapter.maxVertexAttributes: " << _adapterLimits.limits.maxVertexAttributes << "\n";
}

void GraphicsContext::LogAdapterFeatures()
{
    std::vector<wgpu::FeatureName> features;
    size_t featureCount = _adapter.enumerateFeatures(nullptr);
    features.resize(featureCount, wgpu::FeatureName::Undefined);
    _adapter.enumerateFeatures(features.data());

    std::cout << "Adapter Features: \n";
    for (auto const &feature : features)
    {
        std::cout << " - " << feature << "\n";
    }
}

void GraphicsContext::ConfigureDeviceLimits()
{
    // It's best practice to set these as low as possible to alert you when
    // you are using more resources than you want
    _requiredLimits = wgpu::Default;
    _requiredLimits.limits.maxVertexAttributes = 3;
    _requiredLimits.limits.maxVertexBuffers = 1;
    _requiredLimits.limits.maxBufferSize = k_mbBytes;
    _requiredLimits.limits.maxVertexBufferArrayStride = sizeof(Gfx::QuadVertex);
    _requiredLimits.limits.maxInterStageShaderComponents = 6; // everything other than default position needs to be under this max
    _requiredLimits.limits.maxBindGroups = 1;
    _requiredLimits.limits.maxBindingsPerBindGroup = 10;
    _requiredLimits.limits.maxUniformBuffersPerShaderStage = 3;
    _requiredLimits.limits.maxUniformBufferBindingSize = 500 * sizeof(AnimUniform);
    _requiredLimits.limits.maxDynamicUniformBuffersPerPipelineLayout = 1;
    _requiredLimits.limits.maxTextureDimension1D = 16 * k_kbBytes;
    _requiredLimits.limits.maxTextureDimension2D = 16 * k_kbBytes;
    _requiredLimits.limits.maxTextureArrayLayers = k_maxAtlas;
    _requiredLimits.limits.maxSampledTexturesPerShaderStage = 1;
    _requiredLimits.limits.maxSamplersPerShaderStage = 1;

    // Must be set even if we don't use them yet
    _requiredLimits.limits.minStorageBufferOffsetAlignment = _adapterLimits.limits.minStorageBufferOffsetAlignment;
    _requiredLimits.limits.minUniformBufferOffsetAlignment = _adapterLimits.limits.minUniformBufferOffsetAlignment;
}

void GraphicsContext::CreateDevice()
{
    wgpu::DeviceDescriptor deviceDescriptor{};
    deviceDescriptor.label = "Default Device";
    deviceDescriptor.defaultQueue.label = "Default Queue";
    deviceDescriptor.requiredLimits = &_requiredLimits;

    _device = _adapter.requestDevice(deviceDescriptor);

    if (!_device)
    {
        throw std::runtime_error("Could not create WebGPU device");
    }

    _device.getLimits(&_deviceLimits);
    std::cout << "device.maxVertexAttributes: " << _deviceLimits.limits.maxVertexAttributes << "\n";

    _queue = _device.getQueue();

    if (!_queue)
    {
        throw std::runtime_error("Could not get device queue");
    }
}

void GraphicsContext::SetupErrorHandling()
{
    auto onDeviceError = [](wgpu::ErrorType type, char const *message)
    {
        std::cout << "Uncaptured Device error: type-" << type;
        if (message)
            std::cout << " (" << message << ")";
        std::cout << std::endl;//use endl to flush buffer to output

        assert(false);
    };

   _errorCb = _device.setUncapturedErrorCallback(onDeviceError);
}

}
