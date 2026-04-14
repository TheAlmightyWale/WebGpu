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


    void CopyTo(GpuBuffer& gpuBuffer) const{
        //Check gpuBuffer can hold data
        

        //queue up copy
    }

    void Clear() {
        _buffer.clear();
    }

private:
    std::vector<T> _buffer;
};
}