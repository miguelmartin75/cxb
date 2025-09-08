#include <cstddef>
#include <cstdint>

size_t hash(const uint32_t& x);

#include <cxb/cxb.h>

size_t hash(const uint32_t& x) {
    return (size_t) x;
}

static u32 read_u32(const u8*& data, size_t& size) {
    u32 x = 0;
    for(size_t i = 0; i < 4 && i < size; ++i) {
        x |= (u32) data[i] << (i * 8);
    }
    if(size >= 4) {
        data += 4;
        size -= 4;
    } else {
        data += size;
        size = 0;
    }
    return x;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    const u8* data = Data;
    AHashMap<u32, u32> hm;
    while(Size > 0) {
        u8 op = *data;
        data++;
        Size--;
        switch(op % 3) {
            case 0: { // put
                if(Size == 0) return 0;
                u32 key = read_u32(data, Size) % 16;
                u32 value = read_u32(data, Size) % 256;
                hm.put({key, value});
                break;
            }
            case 1: { // get
                u32 key = read_u32(data, Size) % 16;
                if(hm.contains(key)) {
                    volatile u32 v = hm[key];
                    (void) v;
                }
                break;
            }
            case 2: { // erase
                u32 key = read_u32(data, Size) % 16;
                hm.erase(key);
                break;
            }
        }
    }
    return 0;
}
