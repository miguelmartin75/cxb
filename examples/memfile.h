#include <cxb/cxb.h>

struct File {
    void* data;
    size_t len;

    StringSlice filepath;
};

enum class FileOpenErr {
    Success = 0,
    IsNotFile = 1,
    CouldNotOpen = 2,
    Cnt,
};

Result<File*, FileOpenErr> open_file(Arena* arena, StringSlice filepath);
void close_file(File* file);
