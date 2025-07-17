#include "parser.h"

Result<File*, FileOpenErr> open_file(Arena* arena, StringSlice filepath);
void close_file(File* file);

struct Parser {
    // Str8 buffer;
    // size_t idx; // 8
    // SourceLoc loc; // 8
    // Token tok; // 8
    // LexerState state;

    // Token curr_tok;
    // Token next_tok;
};

// static inline Token next_tok(Parser* ctx) {
//     if(ctx->next_tok.kind == TOK_UNINTIALIZED) {
//         ctx->curr_tok = lex_next(ctx);
//     } else {
//         ctx->curr_tok = ctx->next_tok;
//     }
//     ctx->next_tok.kind = TOK_UNINTIALIZED;
//     return ctx->curr_tok;
// }
// 
// static inline Token peek_tok(Parser* ctx) {
//     if(ctx->next_tok.kind == TOK_UNINTIALIZED) {
//         ctx->next_tok = lex_next(ctx);
//     }
//     return ctx->next_tok;
// }

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

Result<File*, FileOpenErr> open_file(Arena* arena, StringSlice filepath) {
    Result<File*, FileOpenErr> result = {};

    int fd = open(filepath.data, O_RDONLY);
    struct stat sb;
    fstat(fd, &sb);
    
    // if (!S_ISREG(sb.st_mode)) {
    if (S_ISDIR(sb.st_mode)) {
        close(fd);
        result.error = FileOpenErr::IsNotFile;
        return result;
    }

    char* data = (char*)mmap((caddr_t)0, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if(!data) {
        result.error = FileOpenErr::CouldNotOpen;
        return result;
    }

    result.value = push<File>(arena);
    result.value->filepath = push_str(arena, filepath);
    result.value->data = data;
    result.value->len = sb.st_size;
    return result;
}

void close_file(File* file) {
    munmap(file->data, file->len);
    file->data = nullptr;
    file->len = 0;
}

C_EXPORT Module* module_make(StringSlice name, Arena* arena, Arena* tree) {
    if(arena == nullptr) {
        // 256KiB for errors
        arena = arena_make_nbytes(sizeof(Module) + MB(256));
    }
    Module* result = push<Module>(arena);
    result->arena = arena;
    result->tree = tree;
    result->name = push_str(arena, name);
    return result;
}

C_EXPORT ParseFileResult module_parse_file(Module* mod, StringSlice file_path) {
    ParseFileResult res = {};
    auto file = open_file(mod->arena, file_path);
    if(!file) {
        res.file_err = file.error;
    }

    mod->file = file.value;
    if(mod->tree == nullptr) {
        // TODO: maybe fix this ratio
        u64 n_bytes = max((u64)mod->file->len, KB(64));
        mod->tree = arena_make_nbytes(n_bytes);
    }
    // TODO: parse
    return res;
}

C_EXPORT void module_destroy(Module* module) {
    arena_destroy(module->tree);
    arena_destroy(module->arena);
}
