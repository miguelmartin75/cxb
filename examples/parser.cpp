#include "lang.h"

struct Parser {
    // Str8 buffer;
    // size_t idx; // 8
    // SourceLoc loc; // 8
    // Token tok; // 8
    // LexerState state;

    // Token curr_tok;
    // Token next_tok;
};

static inline Token next_tok(Parser* ctx) {
    if(ctx->next_tok.kind == TOK_UNINTIALIZED) {
        ctx->curr_tok = lex_next(ctx);
    } else {
        ctx->curr_tok = ctx->next_tok;
    }
    ctx->next_tok.kind = TOK_UNINTIALIZED;
    return ctx->curr_tok;
}

static inline Token peek_tok(Parser* ctx) {
    if(ctx->next_tok.kind == TOK_UNINTIALIZED) {
        ctx->next_tok = lex_next(ctx);
    }
    return ctx->next_tok;
}

Module module_make(Arena* arena, StringSlice name) {
    Module result;
    // result.name = arena_push();
    return result;
}

ParseResult module_parse_file(Module* mod, StringSlice file_path) {
}

ParseResult module_parse_data(Module* mod, StringSlice data, StringSlice module_name) {
}

void module_destroy(Module* module) {
    arena_destroy(&module->tree);
}
