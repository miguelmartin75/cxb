#pragma once

#include "examples/memfile.h"

#include <cxb/cxb.h>

// TODO: CXB_C_EXPORT -> C_EXPORT ?
#define C_EXPORT CXB_C_EXPORT

struct SourceLoc {
    unsigned int line;
    unsigned int col;
};

enum TokenKind {
    // TODO: minimize these ?
    TOK_UNINTIALIZED = 0, // nil state (for compiler)
    TOK_EOF_ = 1,
    TOK_ERROR = 2,             // error
    TOK_STATEMENT_END = 3,     // TODO ; or \n
    TOK_RETURN_KEYWORD = 4,    // return
    TOK_CONST_KEYWORD = 5,     // const
    TOK_VAR_KEYWORD = 6,       // var
    TOK_TYPE_KEYWORD = 7,      // type
    TOK_STRUCT_KEYWORD = 8,    // struct
    TOK_UNION_KEYWORD = 9,     // union
    TOK_ENUM_KEYWORD = 10,     // enum
    TOK_FUNCTION_KEYWORD = 11, // func
    TOK_DEFER_KEYWORD = 12,    // defer
    TOK_IMPORT_KEYWORD = 13,   // import
    TOK_WHILE_KEYWORD = 14,    // while
    TOK_FOR_KEYWORD = 15,      // for
    TOK_CONTINUE_KEYWORD = 16, // continue
    TOK_BREAK_KEYWORD = 17,    // break
    TOK_STATIC_KEYWORD = 18,   // static
    TOK_IF_KEYWORD = 19,       // if
    TOK_ELIF_KEYWORD = 20,     // elif
    TOK_ELSE_KEYWORD = 21,     // else
    TOK_SWITCH_KEYWORD = 22,   // switch
    TOK_CASE_KEYWORD = 23,     // case
    TOK_IDENTIFIER = 24,       // ([a-zA-z])+([0-9])*([a-zA-z])*
    TOK_STRING_LITERAL = 25,   // '<text>' or "<text>"
    TOK_INT_LITERAL = 26,      // 123
    TOK_FLOAT_LITERAL = 27,    // 123.456
    TOK_BOOL_LITERAL = 28,     // false/true
    TOK_NIL_LITERAL = 29,      // nil
    TOK_MAP_OP = 30,           // => TODO rename/change symbol? lambda?
    TOK_NOT_OP = 31,           // not  TODO: reorder priority
    TOK_IMPL_UNARY_OP_BEGIN = 32,
    TOK_IMPL_BIN_OP_BEGIN = 33,
    TOK_MUL_OP = TOK_IMPL_BIN_OP_BEGIN, // *
    TOK_DIV_OP = 34,                    // /
    TOK_MINUS_OP = 35,                  // -
    TOK_PLUS_OP = 36,                   // +
    TOK_IMPL_UNARY_OP_END = TOK_PLUS_OP,
    TOK_LESS_THAN_OP = 37,             // <
    TOK_LESS_THAN_EQUAL_TO_OP = 38,    // <=
    TOK_GREATER_THAN_OP = 39,          // >
    TOK_GREATER_THAN_EQUAL_TO_OP = 40, // >=
    TOK_EQUALITY_OP = 41,              // ==
    TOK_NOT_EQUAL_OP = 42,             // !=
    TOK_AND_OP = 43,                   // 'and' | '&'
    TOK_OR_OP = 44,                    // 'or' | '|'
    TOK_EQUALS_OP = 45,                // =
    TOK_COLON_EQUALS_OP = 46,          // :=
    TOK_PLUS_EQUALS_OP = 47,           // +=
    TOK_MINUS_EQUALS_OP = 48,          // -=
    TOK_MUL_EQUALS_OP = 49,            // *=
    TOK_DIV_EQUALS_OP = 50,            // /=
    TOK_AND_EQUALS_OP = 51,            // &=
    TOK_OR_EQUALS_OP = 52,             // &=
    TOK_IMPL_BIN_OP_END = TOK_AND_EQUALS_OP,

    TOK_DOT_OP = 53,              // .
    TOK_DOUBLE_DOT_OP = 54,       // ..
    TOK_COLON_OP = 55,            // :
    TOK_COMMA = 56,               // ,
    TOK_BRACKET_LEFT = 57,        // (
    TOK_BRACKET_RIGHT = 58,       // )
    TOK_INDEX_BRACKET_LEFT = 59,  // [
    TOK_INDEX_BRACKET_RIGHT = 60, // ]
    TOK_SCOPE_BRACKET_LEFT = 61,  // {
    TOK_SCOPE_BRACKET_RIGHT = 62, // }
    TOK_END,
};

struct Token {
    TokenKind kind : 6;
    u64 idx : 32;
    u64 n : 26;
    u64 line : 32;
    u64 col : 31;
    u64 err : 1;

    inline String8 ss(const char* buffer) const {
        return String8{.data = (char*) (buffer + idx), .len = (size_t) n, .not_null_term = true};
    }

    inline String8 ss(const String8& sv) const {
        DEBUG_ASSERT(n < sv.len);
        return String8{.data = (char*) (sv.data + idx), .len = (size_t) n, .not_null_term = true};
    }
};

enum NodeKind {
    NODE_NONE = 0,
    NODE_BODY = 1,
    NODE_PARAMS_DECL = 2,
    NODE_PARAMS = 3,
    NODE_IDENTIFIER = 4, // tok
    // TODO: NODE_LIT ..?
    // NODE_LIT_BEGIN = NODE_STRING_LIT,
    NODE_STRING_LIT = 5, // lit = tok
    NODE_INT_LIT = 6,    // lit = tok
    NODE_FLOAT_LIT = 7,  // lit = tok
    NODE_BOOL_LIT = 8,   // lit = tok
    NODE_NIL_LIT = 9,    // lit = tok
    // NODE_LIT_END = NODE_FLOAT_LIT,

    // operators
    NODE_BIN_OP = 10,   // tok = op
    NODE_UNARY_OP = 11, // tok = op

    // control flow
    // NODE_CTRL_FLOW_BEGIN = TODO,
    NODE_IF = 12,   // kid(0) == cond, kid(1) == body, kid(2..) == elif/else
    NODE_ELIF = 13, // kid(0) == cond, kid(1) == body, kid(2..) == elif/else
    NODE_ELSE = 14, // kid(0) == body
    NODE_FOR = 15,
    NODE_WHILE = 16,
    NODE_BREAK = 17,
    NODE_CONT = 18,
    NODE_RET = 19,
    NODE_DEFER = 20,
    NODE_FUNC_CALL = 21,
    // NODE_CTRL_FLOW_END = TODO,

    NODE_VAR_DECL = 22, // tok = identifier
    NODE_DECL_BEGIN = NODE_VAR_DECL,
    NODE_TYPEALIAS_DECL = 23, // *tok = type name
    NODE_STRUCT_DECL = 24,    // *tok = name of struct
    NODE_ENUM_DECL = 25,      // *tok = name of enum
    NODE_UNION_DECL = 26,     // *tok = name of union
    NODE_FUNC_DECL = 27,      // tok = name of func
    NODE_DECL_END = NODE_FUNC_DECL,
    // NODE_ROOT_END = NODE_FUNC_DECL,

    NODE_MODULE = 28,
    NODE_IMPORT = 29,
};

enum AstPrimitiveTypes {
    TID_UNDEF = 0,
    TID_FUNC = 1,
    TID_TYPE = 2,
    TID_BOOL = 3,
    TID_INT = 4,
    TID_FLOAT = 5,
    TID_STR = 6,
    TID_NIL = 7,
    TID_VOID = 8,
};

struct AstNode;
struct AstNodeEdgeList {
    struct AstNode** data;
    u64 len;

#ifdef __cplusplus
    inline AstNode* operator[](size_t idx) const {
        return data[idx];
    }
#endif
};

typedef enum {
    UNION_KIND_NAMED,
    UNION_KIND_FLAT,
} UnionKind;

typedef struct NumeralLiteral {
    // TODO ?
    // union {
    //     uint8_t u8;
    //     uint16_t u16;
    //     uint32_t u32;
    //     uint32_t u64;
    //     int8_t i8;
    //     int16_t i16;
    //     int32_t i32;
    //     int64_t i64_t;
    // };
    int64_t value;
} NumeralLiteral;

typedef struct VarDecl {
    // TODO: bitset for modifiers
    bool is_const;
    bool is_ref;
} VarDecl;

typedef struct ParamList {
    // TODO: bitset for modifiers
    bool is_template_args;
} ParamList;

typedef struct FuncDecl {
    // TODO: bitset for modifiers
    bool is_template;
    bool type_ret;
    bool instance_ret;
} FuncDecl;

typedef union AstNodeData {
    NumeralLiteral numeral_literal;
    UnionKind union_kind;
    // StringLiteral string_literal;
    VarDecl var_decl;
    ParamList param_list;
    FuncDecl func_decl;
} AstNodeData;

struct AstNode {
    NodeKind kind; // : 7;
    bool err;      // : 1;

    Token tok;            // 64
    AstNodeData data;     // TODO
    AstNodeEdgeList kids; // 128

    unsigned int scope : 24;
    unsigned int type_id : 30;
    bool comp_time : 1;
    bool statement : 1;
    // TODO expr && statement ?
};

struct ParseError {
    AstNode* root;
    String8 message;
};

struct ParseErrorArray {
    ParseError* data;
    u64 len;
};

struct Parser;
struct Module {
    String8 name;
    MemFile file;

    AstNode* root;

    Parser* parser;
    ParseErrorArray parse_errors;

    Arena* arena;
    Arena* tree;
};

struct ParseFileResult {
    i64 num_errors;
    FileOpenErr file_err;
    String8 message;

#ifdef __cplusplus
    inline operator bool() {
        return !(num_errors == 0 && file_err == FileOpenErr::Success);
    }
#endif
};

C_EXPORT Module* module_make(String8 name, Arena* arena, Arena* tree);
C_EXPORT ParseFileResult module_parse_file(Module* mod, String8 file_path);
C_EXPORT void module_destroy(Module* module);
