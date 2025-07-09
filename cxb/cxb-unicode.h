#pragma once

#include <cxb/cxb.h>

struct Utf8DecodeResult {
    rune codepoint;
    u8 bytes_consumed;
    bool valid;
};

struct Utf8EncodeResult {
    u8 bytes[4];
    u8 byte_count;
    bool valid;
};

CXB_INLINE Utf8DecodeResult utf8_decode(const u8* bytes, size_t max_bytes) {
    if(max_bytes == 0 || bytes == nullptr) {
        return {0, 0, false};
    }

    u8 first = bytes[0];

    // ASCII (0xxxxxxx)
    if((first & 0x80) == 0) {
        return {static_cast<rune>(first), 1, true};
    }

    // Multi-byte sequences
    u8 expected_bytes;
    rune codepoint;

    if((first & 0xE0) == 0xC0) {
        // 2-byte sequence (110xxxxx 10xxxxxx)
        expected_bytes = 2;
        codepoint = first & 0x1F;
    } else if((first & 0xF0) == 0xE0) {
        // 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
        expected_bytes = 3;
        codepoint = first & 0x0F;
    } else if((first & 0xF8) == 0xF0) {
        // 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        expected_bytes = 4;
        codepoint = first & 0x07;
    } else {
        // Invalid first byte
        return {0, 1, false};
    }

    if(max_bytes < expected_bytes) {
        return {0, 0, false};
    }

    for(u8 i = 1; i < expected_bytes; ++i) {
        u8 byte = bytes[i];
        if((byte & 0xC0) != 0x80) {
            return {0, i, false};
        }
        codepoint = (codepoint << 6) | (byte & 0x3F);
    }

    // Check for overlong encodings and invalid codepoints
    if((expected_bytes == 2 && codepoint < 0x80) || (expected_bytes == 3 && codepoint < 0x800) ||
       (expected_bytes == 4 && codepoint < 0x10000) || (codepoint > 0x10FFFF) ||
       (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
        return {0, expected_bytes, false};
    }

    return {codepoint, expected_bytes, true};
}

CXB_INLINE Utf8DecodeResult utf8_decode(const char* str, size_t max_bytes) {
    return utf8_decode(reinterpret_cast<const u8*>(str), max_bytes);
}

CXB_INLINE u8 utf8_sequence_length(u8 first_byte) {
    if((first_byte & 0x80) == 0) return 1;
    if((first_byte & 0xE0) == 0xC0) return 2;
    if((first_byte & 0xF0) == 0xE0) return 3;
    if((first_byte & 0xF8) == 0xF0) return 4;
    return 0; // Invalid
}

CXB_INLINE Utf8EncodeResult utf8_encode(rune codepoint) {
    Utf8EncodeResult result = {{0}, 0, false};

    if(codepoint < 0 || codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
        return result;
    }

    if(codepoint <= 0x7F) {
        // 1-byte sequence
        result.bytes[0] = static_cast<u8>(codepoint);
        result.byte_count = 1;
    } else if(codepoint <= 0x7FF) {
        // 2-byte sequence
        result.bytes[0] = 0xC0 | static_cast<u8>(codepoint >> 6);
        result.bytes[1] = 0x80 | static_cast<u8>(codepoint & 0x3F);
        result.byte_count = 2;
    } else if(codepoint <= 0xFFFF) {
        // 3-byte sequence
        result.bytes[0] = 0xE0 | static_cast<u8>(codepoint >> 12);
        result.bytes[1] = 0x80 | static_cast<u8>((codepoint >> 6) & 0x3F);
        result.bytes[2] = 0x80 | static_cast<u8>(codepoint & 0x3F);
        result.byte_count = 3;
    } else {
        // 4-byte sequence
        result.bytes[0] = 0xF0 | static_cast<u8>(codepoint >> 18);
        result.bytes[1] = 0x80 | static_cast<u8>((codepoint >> 12) & 0x3F);
        result.bytes[2] = 0x80 | static_cast<u8>((codepoint >> 6) & 0x3F);
        result.bytes[3] = 0x80 | static_cast<u8>(codepoint & 0x3F);
        result.byte_count = 4;
    }

    result.valid = true;
    return result;
}

// TODO: optimize with SIMD
template <size_t BufferSize>
struct Utf8IteratorBatched {
    StringSlice s;
    size_t pos = 0;
    rune buffer[BufferSize] = {};

    explicit Utf8IteratorBatched(const StringSlice& s) : s{s}, pos{0}, buffer{{}} {}
    Utf8IteratorBatched(const Utf8IteratorBatched&) = delete;
    Utf8IteratorBatched(Utf8IteratorBatched&&) = delete;

    CXB_INLINE void reset(const StringSlice& s) {
        this->s = s;
        pos = 0;
    }
    CXB_INLINE void reset() {
        pos = 0;
    }
    CXB_INLINE bool has_next() const {
        return pos < s.len;
    }

    CXB_INLINE Utf8DecodeResult next() {
        if(!has_next()) {
            return {0, 0, false};
        }

        auto result = utf8_decode(s.data + pos, s.len - pos);
        if(result.valid) {
            pos += result.bytes_consumed;
        }
        return result;
    }

    CXB_INLINE rune peek() {
        if(!has_next()) return 0;
        auto result = utf8_decode(s.data + pos, s.len - pos);
        return result.valid ? result.codepoint : 0;
    }

    CXB_INLINE size_t remaining_bytes() const {
        return pos < s.len ? s.len - pos : 0;
    }
};

typedef Utf8IteratorBatched<512> Utf8Iterator;
