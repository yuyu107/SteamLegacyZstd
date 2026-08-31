#include <stdint.h>
#include <stddef.h>
#include "zstd.h"

#if defined(_WIN32)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

static uint32_t read_le32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint32_t crc32_bytes(const unsigned char *p, size_t n) {
    uint32_t crc = 0xffffffffU;
    unsigned int bit;
    while (n--) {
        crc ^= *p++;
        for (bit = 0; bit != 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320U & (uint32_t)-(int)(crc & 1));
    }
    return ~crc;
}

/* Decode one complete Steam VSZa wrapper. Returns 1 only for a fully
   validated block, otherwise 0. */
EXPORT int SteamVSZaDecompress(void *dst, uint32_t dst_capacity,
                               const void *wrapped, uint32_t wrapped_size) {
    const unsigned char *src = (const unsigned char *)wrapped;
    size_t got;
    uint32_t expected_size, expected_crc32;

    if (!dst || !src || wrapped_size < 23) return 0;
    if (src[0] != 'V' || src[1] != 'S' || src[2] != 'Z' || src[3] != 'a') return 0;
    if (src[wrapped_size - 3] != 'z' || src[wrapped_size - 2] != 's' ||
        src[wrapped_size - 1] != 'v') return 0;

    expected_crc32 = read_le32(src + wrapped_size - 15);
    expected_size = read_le32(src + wrapped_size - 11);
    if (expected_size == 0 || expected_size > dst_capacity) return 0;

    got = ZSTD_decompress(dst, expected_size, src + 8, wrapped_size - 23);
    if (ZSTD_isError(got) || got != expected_size) return 0;
    if (crc32_bytes((const unsigned char *)dst, got) != expected_crc32) return 0;
    return 1;
}
