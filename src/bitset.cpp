#pragma once

#include <array>
#include <stddef.h>

template <size_t N> class Bitset {
    std::array<uint8_t, N / 8 + 1> arr;

public:
    Bitset()
    {
        this->reset();
    };

    void reset()
    {
        this->arr.fill(0);
    }

    size_t size()
    {
        return N;
    }

    uint8_t* data()
    {
        return this->arr.data();
    }

    bool test(size_t index)
    {
        return this->arr[index / 8] & (1 << (index % 8));
    }
};