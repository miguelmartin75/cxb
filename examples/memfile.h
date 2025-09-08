#pragma once
#include <cxb/cxb.h>

struct MemFile {
    Array<char> data;
    String8 filepath;
};

enum class FileOpenErr {
    Success = 0,
    IsNotFile = 1,
    CouldNotOpen = 2,
    Cnt,
};

Result<MemFile, FileOpenErr> open_memfile(Arena* arena, String8 filepath);
void close_memfile(MemFile& file);
