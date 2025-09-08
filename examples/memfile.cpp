#include "memfile.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

Result<MemFile, FileOpenErr> open_memfile(Arena* arena, String8 filepath) {
    Result<MemFile, FileOpenErr> result = {};

    int fd = open(filepath.data, O_RDONLY);
    struct stat sb;
    fstat(fd, &sb);

    if(S_ISDIR(sb.st_mode)) {
        close(fd);
        result.error = FileOpenErr::IsNotFile;
        return result;
    }

    char* data = (char*) mmap((caddr_t) 0, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if(data == MAP_FAILED) {
        result.error = FileOpenErr::CouldNotOpen;
        return result;
    }

    result.value.filepath = arena_push_string8(arena, filepath);
    result.value.data.data = data;
    result.value.data.len = sb.st_size;
    return result;
}

void close_memfile(MemFile& file) {
    munmap(file.data.data, file.data.len);
    file.data.data = nullptr;
    file.data.len = 0;
}
