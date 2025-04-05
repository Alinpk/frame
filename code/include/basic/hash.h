#pragma once

#include <cstdint>

namespace XH {
enum class hash_algorithm
{
    murmur3_32,
};

template <typename T, std::size_t byte_width, hash_algorithm alg = hash_algorithm::murmur3_32>
struct hash
{
    [[nodiscard]]uint32_t operator()(const T& key) noexcept;
    constexpr static uint32_t seed = 0x9747b28c;
};

// MurmurHash3_32
template <typename T, std::size_t byte_width>
uint32_t MurmurHash3_32(const T& key, uint32_t seed)
{
    const uint8_t* data = (const uint8_t*)key.data();
    const int nblocks = key.size() * byte_width / 4;

    uint32_t h1 = seed;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    // 处理每4字节的数据块
    const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4);
    for (int i = -nblocks; i; i++)
    {
        uint32_t k1 = blocks[i];

        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> (32 - 15));
        k1 *= c2;

        h1 ^= k1;
        h1 = (h1 << 13) | (h1 >> (32 - 13));
        h1 = h1 * 5 + 0xe6546b64;
    }

    // 处理剩余的字节
    const uint8_t* tail = (const uint8_t*)(data + nblocks * 4);
    uint32_t k1 = 0;

    switch (key.size() * byte_width & 3)
    {
    case 3:
        k1 ^= tail[2] << 16;
    case 2:
        k1 ^= tail[1] << 8;
    case 1:
        k1 ^= tail[0];
        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> (32 - 15));
        k1 *= c2;
        h1 ^= k1;
    };

    // 最终的混合操作
    h1 ^= key.size() * byte_width;
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return h1;
}

template <typename T, std::size_t byte_width, hash_algorithm alg>
[[nodiscard]] uint32_t hash<T, byte_width, alg>::operator()(const T& key) noexcept
{
    if constexpr (alg == hash_algorithm::murmur3_32)
        return MurmurHash3_32<T, byte_width>(key, seed);
    else
        return 0;
}
} // namespace XH