#pragma once
#include <cxb/cxb.h>

struct File {
    Array<char> data;
    String8 filepath;
};

enum class FileOpenErr {
    Success = 0,
    IsNotFile = 1,
    CouldNotOpen = 2,
    Cnt,
};

Result<File, FileOpenErr> open_file(Arena* arena, String8 filepath);
void close_file(File& file);
