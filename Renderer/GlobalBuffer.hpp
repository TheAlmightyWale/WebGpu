#pragma once
#include "CpuBuffer.hpp"

namespace Gfx
{
template<typename T>
class GlobalBuffer{
public:
    static uint32_t const k_maxSize = 1024;

    static CpuBuffer<T>& Get(){
        static GlobalBuffer<T> global(k_maxSize);
        return global._buffer;
    }

    ~GlobalBuffer() = default;


    CpuBuffer<T> _buffer;

private:
    template <typename... Args>
    GlobalBuffer(Args&&... args)
    : _buffer(std::forward<Args>(args)...)
    {}


};
}