#include "memfile.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

Result<File*, FileOpenErr> open_file(Arena* arena, String8 filepath) {
    Result<File*, FileOpenErr> result = {};

    int fd = open(filepath.data, O_RDONLY);
    struct stat sb;
    fstat(fd, &sb);

    // if (!S_ISREG(sb.st_mode)) {
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

    result.value = arena_push<File>(arena);
    result.value->filepath = arena_push_string8(arena, filepath);
    result.value->data = data;
    result.value->len = sb.st_size;
    return result;
}

void close_file(File* file) {
    munmap(file->data, file->len);
    file->data = nullptr;
    file->len = 0;
}
