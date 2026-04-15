#pragma once
#include "GpuBuffer.h"
#include <vector>
#include <span>

namespace Gfx
{
//Manager class for a buffer of Type T
// Provides methods for adding data and copying to GPU
template<typename T>
class CpuBuffer
{
public:
    CpuBuffer(uint32_t maxElements)
        :_buffer()
    {
        _buffer.reserve(maxElements);
    }

    ~CpuBuffer() = default;

    //returns a span of data that the caller now controls
    // if we cannot allocate numElements then returns an empty span
    std::span<T> RegisterData(uint32_t numElements){
        if(_buffer.size() + numElements > _buffer.capacity()){
            return {};
        }

        size_t currentSize = _buffer.size();
        _buffer.resize(currentSize + numElements);

        return std::span(_buffer.begin() + currentSize, _buffer.end());
    }

    void CopyTo(GpuBuffer& gpuBuffer, wgpu::Queue queue) const{
        assert(gpuBuffer.Size() >= SizeBytes());

        gpuBuffer.EnqueueCopy(_buffer.data(), SizeBytes(), 0, queue);
    }

    void Clear() {
        _buffer.clear();
    }

    inline uint32_t Size() const{
        return (uint32_t)_buffer.size();
    }

    inline uint32_t SizeBytes() const{
        return (uint32_t)_buffer.size() * sizeof(T);
    }

private:
    std::vector<T> _buffer;
};
}