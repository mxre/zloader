#pragma once

[[ gnu::pure, maybe_unused ]] static inline uint8_t minU8(uint8_t a, uint8_t b) { return a < b ? a : b; }
[[ gnu::pure, maybe_unused ]] static inline uint16_t minU16(uint16_t a, uint16_t b) { return a < b ? a : b; }
[[ gnu::pure, maybe_unused ]] static inline uint32_t minU32(uint32_t a, uint32_t b) { return a < b ? a : b; }
[[ gnu::pure, maybe_unused ]] static inline uint64_t minU64(uint64_t a, uint64_t b) { return a < b ? a : b; }

#define MIN(a, b) _Generic((a), \
    uint8_t: minU8(a, b), \
    uint16_t: minU16(a, b), \
    uint32_t: minU32(a, b), \
    uint64_t: minU64(a, b)  )

[[ gnu::pure, maybe_unused ]] static inline uint8_t maxU8(uint8_t a, uint8_t b) { return a > b ? a : b; }
[[ gnu::pure, maybe_unused ]] static inline uint16_t maxU16(uint16_t a, uint16_t b) { return a > b ? a : b; }
[[ gnu::pure, maybe_unused ]] static inline uint32_t maxU32(uint32_t a, uint32_t b) { return a > b ? a : b; }
[[ gnu::pure, maybe_unused ]] static inline uint64_t maxU64(uint64_t a, uint64_t b) { return a > b ? a : b; }

#define MAX(a, b) _Generic((a), \
    uint8_t: maxU8(a, b), \
    uint16_t: maxU16(a, b), \
    uint32_t: maxU32(a, b), \
    uint64_t: maxU64(a, b)  )
