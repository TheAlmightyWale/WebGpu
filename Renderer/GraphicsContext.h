#pragma once
#include "Core/webgpu.h"
#include "Renderer.h"

// Encapsulates WebGPU instance, adapter, device, queue, and surface initialization.
// Handles all GPU resource setup and provides access to the core WebGPU objects.
class GraphicsContext
{
public:
    // Initializes WebGPU instance, adapter, device, queue, and surface for the given window.
    // Throws std::runtime_error if initialization fails.
    explicit GraphicsContext(Window &window);

    // Releases all WebGPU resources in proper order
    ~GraphicsContext();

    // Non-copyable, non-movable
    GraphicsContext(GraphicsContext const &) = delete;
    GraphicsContext &operator=(GraphicsContext const &) = delete;
    GraphicsContext(GraphicsContext &&) = delete;
    GraphicsContext &operator=(GraphicsContext &&) = delete;

    // Accessors for WebGPU objects
    wgpu::Device GetDevice() const { return _device; }
    wgpu::Queue GetQueue() const { return _queue; }
    wgpu::Adapter GetAdapter() const { return _adapter; }
    wgpu::Instance GetInstance() const { return _instance; }
    wgpu::Surface GetSurface() const { return _surface; }

    // Returns the minimum uniform buffer offset alignment required by the device
    uint32_t GetUniformAlignment() const { return _deviceLimits.limits.minUniformBufferOffsetAlignment; }

    // Returns the full device limits structure
    wgpu::SupportedLimits const &GetDeviceLimits() const { return _deviceLimits; }

private:
    void InitializeInstance();
    void CreateSurface(Window &window);
    void RequestAdapter();
    void LogAdapterFeatures();
    void ConfigureDeviceLimits();
    void CreateDevice();
    void SetupErrorHandling();

    wgpu::Instance _instance;
    wgpu::Surface _surface;
    wgpu::Adapter _adapter;
    wgpu::Device _device;
    wgpu::Queue _queue;

    wgpu::SupportedLimits _adapterLimits;
    wgpu::SupportedLimits _deviceLimits;
    wgpu::RequiredLimits _requiredLimits;
};
