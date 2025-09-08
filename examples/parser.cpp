#include "parser.h"

// TODO add option to prefill
// #define TOKENIZER_PREFILL

struct Parser {
    String8 buffer;
    size_t idx; // lexer idx
    SourceLoc loc;
    Token tok;
    Token curr_tok;
    Token next_tok;

    Arena* tree;
    Arena* error_arena;
    ParseErrorArray errors;
};

static Token lex_next(Parser* ctx);
static Optional<i128> atoi128(String8 str);
static AstNode* parse_module(Parser* ctx);

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

C_EXPORT Module* module_make(String8 name, Arena* arena, Arena* tree) {
    if(arena == nullptr) {
        // 256KiB for errors
        arena = arena_make_nbytes(sizeof(Module) + MB(256));
    }
    Module* result = arena_push<Module>(arena);
    result->arena = arena;
    result->tree = tree;
    result->name = arena_push_string8(arena, name);
    result->parser = arena_push<Parser>(arena);
    return result;
}

C_EXPORT ParseFileResult module_parse_file(Module* mod, String8 file_path) {
    ParseFileResult res = {};
    auto file = open_memfile(mod->arena, file_path);
    if(file) {
        res.file_err = file.error;
        return res;
    }

    mod->file = file.value;
    if(mod->tree == nullptr) {
        // TODO: test this ratio
        u64 n_bytes = max((u64) (1 * mod->file.data.len), KB(64));
        mod->tree = arena_make_nbytes(n_bytes);
    }

    DEBUG_ASSERT(mod->parser);
    Parser* parser = mod->parser;
    *parser = {};
    parser->idx = 0;
    parser->buffer = mod->file.data.as_string8(false);
    parser->tree = mod->tree;
    parser->error_arena = mod->arena;
    mod->root = parse_module(mod->parser);

    res.num_errors = mod->parse_errors.len;
    return res;
}

C_EXPORT void module_destroy(Module* module) {
    arena_destroy(module->tree);
    arena_destroy(module->arena);
}

// *SECTION: Parser
#define PEEK peek_tok(ctx)
#define NODE(x) (*x)
#define LHS(x) NODE(NODE(x).kids.data[0])
#define RHS(x) NODE(NODE(x).kids.data[1])
#define ADD_KID(x, k) array_push_back((x)->kids, ctx->tree, static_cast<AstNode*>(k))
#define ADD_KID_A(x, k, a) array_push_back((x)->kids, a, static_cast<AstNode*>(k))
#define INVALID_NODE nullptr

#define ADD_ERR(node, err_msg, ...)                                                  \
    {                                                                                \
        node->err = 1;                                                               \
        array_push_back(ctx->errors, ctx->error_arena, ParseError{(node), err_msg}); \
    }

#define ADD_ERR_NO_NODE(err_msg, ...)                                                 \
    { array_push_back(ctx->errors, ctx->error_arena, ParseError{nullptr, err_msg}); }

#define TO_STR(x) #x
#define STR_CONCAT(a, b) (a b)

#define NEXT next_tok(ctx)
#define CONSUME (void) NEXT
#define CHECK(x, err_msg)         \
    if(!(x)) {                    \
        ADD_ERR_NO_NODE(err_msg); \
    }
#define CHECK_WN(x, err_msg, node) \
    if(!(x)) {                     \
        ADD_ERR(node, err_msg);    \
    }
#define EXPECT(tok, expected) CHECK((tok).kind == (expected), S8_LIT(STR_CONCAT("expected: ", TO_STR(x))))
#define EXPECT_WN(tok, expected, node)                                                                \
    CHECK_WN((tok).kind == (expected), (S8_LIT("unexpected token - TODO list expected token")), node)
#define CONSUME_EXPECT(x) CHECK(NEXT.kind == (x), S8_LIT(STR_CONCAT("expected: ", TO_STR(x))))
#define CONSUME_EXPECT_WN(x, node) CHECK_WN(NEXT.kind == (x), S8_LIT(STR_CONCAT("expected: ", TO_STR(x))), node)

static void parse_statement_list(Parser* ctx, AstNode* node, bool explicit_scope);
static AstNode* parse_expression(Parser* ctx);
static inline bool is_binary_op(TokenKind type) {
    return (int) type >= (int) TOK_IMPL_BIN_OP_BEGIN && (int) type <= (int) TOK_IMPL_BIN_OP_END;
}

static inline bool is_unary_op(TokenKind type) {
    return type >= TOK_IMPL_UNARY_OP_BEGIN && type <= TOK_IMPL_UNARY_OP_END;
}

static inline AstNode* add_node(Parser* ctx, NodeKind kind, AstNodeData data, Token tok = Token{}, bool err = false) {
    AstNode* result = arena_push(ctx->tree,
                                 1,
                                 AstNode{.kind = kind,
                                         .err = err,
                                         .tok = tok,
                                         .data = data,
                                         .kids = {}, // empty arr
                                         .scope = 0, // TODO
                                         .type_id = 0,
                                         .comp_time = 0,
                                         .statement = false});
    return result;
}

static AstNode* parse_expression_base(Parser* ctx) {
    auto next_token = peek_tok(ctx);

    if(next_token.kind == TOK_BRACKET_LEFT) {
        CONSUME;
        AstNode* expr = parse_expression(ctx);
        if(PEEK.kind != TOK_BRACKET_RIGHT) {
            ADD_ERR(expr, S8_LIT("unmatched bracket"));
            return expr;
        }
        CONSUME;
        return expr;
    } else if(next_token.kind == TOK_SCOPE_BRACKET_LEFT) {
        AstNode* body = add_node(ctx, NODE_BODY, {});
        Token scope_bracket = NEXT;
        EXPECT_WN(scope_bracket, TOK_SCOPE_BRACKET_LEFT, body);
        parse_statement_list(ctx, body, true);
        return body;
    } else if(is_unary_op(next_token.kind)) {
        Token op = NEXT;
        AstNode* node = add_node(ctx, NODE_UNARY_OP, {}, op);
        AstNode* expr = parse_expression_base(ctx);
        ADD_KID(node, expr);
        return node;
    } else if(next_token.kind == TOK_IMPORT_KEYWORD) {
        CONSUME;
        // TODO: features (future)
        // 1. import
        //      <a>,
        //      <b>,
        //      <c>
        // 2. import <a> as <b>
        // 3. cimport ...?
        Token identifier = NEXT;
        AstNode* node = add_node(ctx, NODE_IMPORT, {}, identifier);
        EXPECT_WN(identifier, TOK_IDENTIFIER, node);
        return node;
    } else if(next_token.kind == TOK_RETURN_KEYWORD) {
        CONSUME;
        AstNode* node = add_node(ctx, NODE_RET, {});
        if(PEEK.kind != TOK_STATEMENT_END) {
            AstNode* body = parse_expression(ctx);
            ADD_KID(node, body);
        }
        return node;
    } else if(next_token.kind == TOK_DEFER_KEYWORD) {
        CONSUME;
        AstNode* node = add_node(ctx, NODE_DEFER, {});
        AstNode* body = parse_expression(ctx);
        ADD_KID(node, body);
        return node;
    } else if(next_token.kind == TOK_CONTINUE_KEYWORD) {
        CONSUME;
        CONSUME_EXPECT(TOK_STATEMENT_END);
        AstNode* node = add_node(ctx, NODE_CONT, {});
        return node;
    } else if(next_token.kind == TOK_BREAK_KEYWORD) {
        CONSUME;
        CONSUME_EXPECT(TOK_STATEMENT_END);
        AstNode* node = add_node(ctx, NODE_BREAK, {});
        return node;
    } else if(next_token.kind == TOK_WHILE_KEYWORD) {
        CONSUME;
        AstNode* node = add_node(ctx, NODE_WHILE, {});
        AstNode* cond = parse_expression(ctx);
        AstNode* body = parse_expression(ctx);
        ADD_KID(node, cond);
        ADD_KID(node, body);
        return node;
    } else if(next_token.kind == TOK_FOR_KEYWORD) {
        // TODO
        CONSUME;
        ADD_ERR_NO_NODE(S8_LIT("for is unsupported - TODO"));
    } else if(next_token.kind == TOK_IF_KEYWORD) {
        CONSUME;
        AstNode* node = add_node(ctx, NODE_IF, {});
        AstNode* cond = parse_expression(ctx);
        // if 1 == 2 "abc" else "def"
        // if 1 == 2: "abc" else "def"
        // if 1 == 2: { "abc" } else { "def" }
        // TODO: do we care about handling if(x): y ? vs. if(x) y
        if(PEEK.kind == TOK_COLON_OP) {
            CONSUME;
        }
        AstNode* body = parse_expression(ctx);
        ADD_KID(node, cond);
        ADD_KID(node, body);
        while(PEEK.kind == TOK_ELSE_KEYWORD) {
            CONSUME;
            if(PEEK.kind == TOK_IF_KEYWORD) {
                CONSUME;
                AstNode* elif = add_node(ctx, NODE_ELIF, {});
                AstNode* cond = parse_expression(ctx);
                // TODO: do we care about handling if(x): y ? vs. if(x) y
                // TODO: y if x else z if no colon
                // TODO: if x: y else: z if colon
                if(PEEK.kind == TOK_COLON_OP) {
                    CONSUME;
                }
                AstNode* body = parse_expression(ctx);
                ADD_KID(elif, cond);
                ADD_KID(elif, body);
                ADD_KID(node, elif);
            } else {
                AstNode* el = add_node(ctx, NODE_ELSE, {});
                AstNode* body = parse_expression(ctx);
                ADD_KID(el, body);
                ADD_KID(node, el);
                if(PEEK.kind == TOK_ELSE_KEYWORD) {
                    // TODO: error
                }
                break;
            }
        }
        return node;
    } else if(next_token.kind == TOK_IDENTIFIER) {
        Token id = NEXT;
        if(PEEK.kind == TOK_COLON_OP) {
            CONSUME;
            AstNode* ret = add_node(ctx, NODE_VAR_DECL, {}, id);
            AstNode* type_expr = parse_expression_base(ctx);
            if(PEEK.kind == TOK_EQUALS_OP || PEEK.kind == TOK_COLON_EQUALS_OP) {
                CONSUME;
                AstNode* assign = parse_expression(ctx);
                ADD_KID(ret, type_expr);
                ADD_KID(ret, assign);
            } else {
                ADD_KID(ret, type_expr);
                ADD_KID(ret, INVALID_NODE);
            }
            return ret;
        } else if(PEEK.kind == TOK_COLON_EQUALS_OP) {
            // TODO: add :: for constants ?
            CONSUME;
            AstNode* ret = add_node(ctx, NODE_VAR_DECL, {.var_decl = {.is_const = false}}, id);
            AstNode* assign = parse_expression(ctx);
            ADD_KID(ret, INVALID_NODE);
            ADD_KID(ret, assign);
            return ret;
        } else if(PEEK.kind == TOK_BRACKET_LEFT) {
            // NOTE:
            // We might be able to do something that integrates with the
            // syntax better here.
            //
            // For example:
            // - we could parse a tuple here for the args and this could be a
            //   primitive defined in the language
            // - everything in () could be considered a new scope?
            //
            // NOTE: cannot handle no brackets
            CONSUME;
            AstNode* fn = add_node(ctx, NODE_FUNC_CALL, {}, id);
            AstNode* args = add_node(ctx, NODE_PARAMS, {});
            ADD_KID(fn, args);

            while(PEEK.kind != TOK_BRACKET_RIGHT) {
                // must handle binops in args and etc.
                AstNode* arg = parse_expression(ctx);
                ADD_KID(args, arg);
                if(PEEK.kind == TOK_COMMA) {
                    CONSUME;
                } else {
                    /* fall through to outside of loop */
                }
            }
            CONSUME_EXPECT_WN(TOK_BRACKET_RIGHT, args);
            return fn;
        } else {
            return add_node(ctx, NODE_IDENTIFIER, {}, id);
        }
    } else if(next_token.kind == TOK_FLOAT_LITERAL) {
        // TODO: parse float
        Token lit = NEXT;
        auto literal_token = lit.ss(ctx->buffer);
        AstNode* node = add_node(ctx, NODE_INT_LIT, {}, lit);
        if(!lit.err) {
            auto literal = atoi128(literal_token);
            if(!literal.exists) {
                NODE(node).err = 1;
                ADD_ERR(node, S8_LIT("literal not valid"));
            } else {
                NODE(node).data.numeral_literal.value = literal.value;
            }
        } else {
            NODE(node).err = 1;
        }
        return node;
    } else if(next_token.kind == TOK_BOOL_LITERAL) {
        Token lit = NEXT;
        AstNode* node = add_node(ctx, NODE_BOOL_LIT, {}, lit);
        NODE(node).data.numeral_literal.value = lit.ss(ctx->buffer)[0] == 't';
        return node;
    } else if(next_token.kind == TOK_NIL_LITERAL) {
        Token lit = NEXT;
        AstNode* node = add_node(ctx, NODE_NIL_LIT, {}, lit);
        return node;
    } else if(next_token.kind == TOK_INT_LITERAL) {
        Token lit = NEXT;
        auto int_literal_token = lit.ss(ctx->buffer);
        AstNode* node = add_node(ctx, NODE_INT_LIT, {}, lit);
        if(!lit.err) {
            auto literal = atoi128(int_literal_token);
            if(!literal.exists) {
                NODE(node).err = 1;
                ADD_ERR(node, S8_LIT("literal not valid"));
            } else {
                NODE(node).data.numeral_literal.value = literal.value;
            }
        } else {
            NODE(node).err = 1;
        }
        return node;
    } else if(next_token.kind == TOK_STRING_LITERAL) {
        Token tok = NEXT;
        return add_node(ctx, NODE_STRING_LIT, {}, tok, tok.err);
    } else if(next_token.kind != TOK_STATEMENT_END && next_token.kind != TOK_EOF_) {
        ADD_ERR_NO_NODE(S8_LIT("unexpected token"));
    }
    return INVALID_NODE;
}

AstNode* parse_expression(Parser* ctx) {
    AstNode* lhs = parse_expression_base(ctx); // 2

    TokenKind lhs_op = TOK_UNINTIALIZED;
    while(is_binary_op(PEEK.kind)) {
        auto op_token = NEXT; // +
        AstNode* rhs = parse_expression_base(ctx);

        AstNode* curr = add_node(ctx, NODE_BIN_OP, {}, op_token);
        ADD_KID(curr, lhs);
        ADD_KID(curr, rhs);

        // ops to be evaluated first have a smaller integer value
        // NOTE: the initial state will always cause this to be false
        if(lhs_op > op_token.kind) {
            // transform the tree such that the higher priority op is the deeper
            // node (will be evaluated first)
            NODE(curr).kids.data[0] = NODE(lhs).kids.data[1]; // curr.lhs = lhs.rhs
            NODE(lhs).kids.data[1] = curr;                    // lhs.rhs = curr
            lhs_op = NODE(lhs).tok.kind;

            // update error flags
            NODE(curr).err = LHS(curr).err || RHS(curr).err;
            NODE(lhs).err = RHS(lhs).err || RHS(lhs).err;
        } else {
            lhs = curr;
            lhs_op = op_token.kind;

            NODE(curr).err = LHS(curr).err || RHS(curr).err;
        }
    }

    return lhs;
}

AstNode* parse_type_decl(Parser* ctx) {
    CONSUME_EXPECT(TOK_TYPE_KEYWORD);

    Token name = NEXT;
    EXPECT(name, TOK_IDENTIFIER);

    CONSUME_EXPECT(TOK_EQUALS_OP);

    NodeKind nk = {};
    switch(PEEK.kind) {
        case TOK_UNION_KEYWORD:
            nk = NODE_UNION_DECL;
            CONSUME;
            break;
        case TOK_ENUM_KEYWORD:
            nk = NODE_ENUM_DECL;
            CONSUME;
            break;
        case TOK_STRUCT_KEYWORD:
            nk = NODE_STRUCT_DECL;
            CONSUME;
            break;
        case TOK_SCOPE_BRACKET_LEFT:
            nk = NODE_STRUCT_DECL;
            break;
        default:
            nk = NODE_TYPEALIAS_DECL;
            break;
    }
    AstNode* node = add_node(ctx, nk, {}, name);
    switch(nk) {
        case NODE_STRUCT_DECL:
        case NODE_ENUM_DECL:
        case NODE_UNION_DECL: {
            CONSUME_EXPECT(TOK_SCOPE_BRACKET_LEFT);
            while(PEEK.kind != TOK_SCOPE_BRACKET_RIGHT) {
                Token field_name = NEXT;
                EXPECT(field_name, TOK_IDENTIFIER);
                CONSUME_EXPECT_WN(TOK_COLON_OP, node);

                AstNode* field = add_node(ctx, NODE_VAR_DECL, {}, field_name);
                ADD_KID(node, field);

                AstNode* field_type = parse_expression_base(ctx);
                ADD_KID(field, field_type);

                // default assignment
                if(PEEK.kind == TOK_EQUALS_OP) {
                    AstNode* assign_expr = parse_expression(ctx);
                    ADD_KID(field, assign_expr);
                } else {
                    ADD_KID(field, INVALID_NODE);
                }

                if(PEEK.kind != TOK_SCOPE_BRACKET_RIGHT) {
                    CONSUME_EXPECT_WN(TOK_STATEMENT_END, node);
                }
            }
            CONSUME_EXPECT_WN(TOK_SCOPE_BRACKET_RIGHT, node);
            return node;
        } break;
        default: {
            AstNode* expr = parse_expression(ctx);
            switch(NODE(expr).kind) {
                case NODE_BIN_OP: {
                    size_t n = NODE(expr).kids.len;
                    for(size_t i = 0; i < n; ++i) {
                        AstNode* kid = NODE(expr).kids.data[i];
                        if(NODE(kid).kind == NODE_BIN_OP) {
                            EXPECT_WN(NODE(kid).tok, TOK_OR_OP, kid);
                        }
                    }
                    NODE(node).kind = NODE_UNION_DECL;
                    NODE(node).data.union_kind = UNION_KIND_FLAT;
                    ADD_KID(node, expr);
                    // TODO: why do we not expect TOK_STATEMENT_END here?
                } break;
                case NODE_IDENTIFIER: {
                    ADD_KID(node, expr);
                    CONSUME_EXPECT_WN(TOK_STATEMENT_END, node);
                } break;
                default:
                    ADD_ERR(node, S8_LIT("unexpected expression for typedecl"));
                    break;
            }

            return node;
        } break;
    }
    return INVALID_NODE;
}

static AstNode* parse_var_decl(Parser* ctx) {
    //    root = name/identifier
    // kids[0] = type or INVALID_NODE
    // kids[1] = assignment or INVALID_NODE
    Token tok = NEXT;
    Token name = tok;
    if(tok.kind == TOK_VAR_KEYWORD || tok.kind == TOK_CONST_KEYWORD) {
        name = NEXT;
    }
    EXPECT(name, TOK_IDENTIFIER);

    AstNode* node = add_node(ctx, NODE_VAR_DECL, {}, name);
    NODE(node).data.var_decl.is_const = tok.kind == TOK_CONST_KEYWORD;
    if(PEEK.kind == TOK_COLON_OP) {
        CONSUME;
        // NOTE:
        // - if identifier => direct type identifier
        // - type may be returned from expression
        AstNode* expr = parse_expression_base(ctx);
        ADD_KID(node, expr);
        if(PEEK.kind == TOK_COLON_EQUALS_OP || PEEK.kind == TOK_EQUALS_OP) {
            CONSUME;
            AstNode* expr = parse_expression(ctx);
            ADD_KID(node, expr);
        } else {
            ADD_KID(node, INVALID_NODE);
        }
    } else {
        ADD_KID(node, INVALID_NODE);
        if(PEEK.kind == TOK_COLON_EQUALS_OP) {
            CONSUME;
            AstNode* expr = parse_expression(ctx);
            ADD_KID(node, expr);
        } else if(PEEK.kind == TOK_EQUALS_OP) {
            CONSUME;
            AstNode* expr = parse_expression(ctx);
            ADD_KID(node, expr);
        } else {
            ADD_KID(node, INVALID_NODE);
        }
    }
    return node;
}

static void parse_args_decl(Parser* ctx, AstNode* node) {
    while(PEEK.kind != TOK_BRACKET_RIGHT) {
        AstNode* var_decl = parse_var_decl(ctx);
        ADD_KID(node, var_decl);
        if(PEEK.kind == TOK_COMMA) {
            CONSUME;
        }
    }
}

static AstNode* parse_func_decl(Parser* ctx) {
    // root.name = name
    // kids[0] = targs
    // kids[1] = args
    // kids[2] = body
    CONSUME_EXPECT(TOK_FUNCTION_KEYWORD);

    Token name = NEXT;
    EXPECT(name, TOK_IDENTIFIER);

    AstNode* node = add_node(ctx, NODE_FUNC_DECL, {}, name);
    AstNode* args = add_node(ctx, NODE_PARAMS_DECL, {.param_list = {.is_template_args = false}});
    AstNode* targs = add_node(ctx, NODE_PARAMS_DECL, {.param_list = {.is_template_args = true}});

    CONSUME_EXPECT(TOK_BRACKET_LEFT);
    parse_args_decl(ctx, args);
    CONSUME_EXPECT(TOK_BRACKET_RIGHT);

    // TODO: check for another left bracket for previous as template args
    // func foo[X, T](x: int, y: int)
    if(PEEK.kind == TOK_BRACKET_LEFT) {
        CONSUME;
        parse_args_decl(ctx, targs);
        CONSUME_EXPECT(TOK_BRACKET_RIGHT);
    }

    AstNode* body = add_node(ctx, NODE_BODY, {});
    CONSUME_EXPECT(TOK_SCOPE_BRACKET_LEFT);
    parse_statement_list(ctx, body, true);

    ADD_KID(node, targs);
    ADD_KID(node, args);
    ADD_KID(node, body);
    return node;
}

static AstNode* parse_statement(Parser* ctx) {
    switch(PEEK.kind) {
        case TOK_TYPE_KEYWORD:
            return parse_type_decl(ctx);
        case TOK_FUNCTION_KEYWORD:
            return parse_func_decl(ctx);
        case TOK_CONST_KEYWORD:
        case TOK_VAR_KEYWORD:
            return parse_var_decl(ctx);
        default:
            return parse_expression(ctx);
    }
    return parse_expression(ctx);
}

static void parse_statement_list(Parser* ctx, AstNode* node, bool explicit_scope) {
    Arena* scratch = arena_make_nbytes(KB(16)); // TODO: get a scratch arena
    while(true) {
        while(peek_tok(ctx).kind == TOK_STATEMENT_END) {
            CONSUME;
        }
        Token p = peek_tok(ctx);
        if(p.kind == TOK_EOF_) {
            CONSUME;
            break;
        } else if(explicit_scope && p.kind == TOK_SCOPE_BRACKET_RIGHT) {
            CONSUME;
            break;
        }

        AstNode* statement = parse_statement(ctx);
        if(statement == INVALID_NODE) {
            break;
        }
        NODE(statement).statement = true;
        NODE(node).err |= NODE(statement).err;
        ADD_KID_A(node, statement, scratch);
    }
    // TODO: copy node->kids to main arena ?
}

static AstNode* parse_module(Parser* ctx) {
    AstNode* node = add_node(ctx, NODE_MODULE, {});
    parse_statement_list(ctx, node, false);
    return node;
}

#undef PEEK
#undef ADD_ERR
#undef NEXT
#undef CONSUME
#undef CHECK
#undef EXPECT

// *SECTION: Lexer
#define ALPHA       \
    'A' : case 'B': \
    case 'C':       \
    case 'D':       \
    case 'E':       \
    case 'F':       \
    case 'G':       \
    case 'H':       \
    case 'I':       \
    case 'J':       \
    case 'K':       \
    case 'L':       \
    case 'M':       \
    case 'N':       \
    case 'O':       \
    case 'P':       \
    case 'Q':       \
    case 'R':       \
    case 'S':       \
    case 'T':       \
    case 'U':       \
    case 'V':       \
    case 'W':       \
    case 'X':       \
    case 'Y':       \
    case 'Z':       \
    case 'a':       \
    case 'b':       \
    case 'c':       \
    case 'd':       \
    case 'e':       \
    case 'f':       \
    case 'g':       \
    case 'h':       \
    case 'i':       \
    case 'j':       \
    case 'k':       \
    case 'l':       \
    case 'm':       \
    case 'n':       \
    case 'o':       \
    case 'p':       \
    case 'q':       \
    case 'r':       \
    case 's':       \
    case 't':       \
    case 'u':       \
    case 'v':       \
    case 'w':       \
    case 'x':       \
    case 'y':       \
    case 'z'

#define DIGIT       \
    '0' : case '1': \
    case '2':       \
    case '3':       \
    case '4':       \
    case '5':       \
    case '6':       \
    case '7':       \
    case '8':       \
    case '9'

#define CONSUME       \
    ctx->idx += 1;    \
    ctx->loc.col += 1
#define CONSUME_X(x)  \
    ctx->idx += x;    \
    ctx->loc.col += x
#define CONSUME_NEXT_LINE \
    ctx->idx += 1;        \
    ctx->loc.line += 1;   \
    ctx->loc.col = 0
#define PEEK (ctx->idx + 1 < ctx->buffer.len ? ctx->buffer[ctx->idx + 1] : 0)
#define ADD_TOK(x)                     \
    {                                  \
        ctx->tok.idx = ctx->idx;       \
        ctx->tok.kind = (x);           \
        ctx->tok.line = ctx->loc.line; \
        ctx->tok.col = ctx->loc.col;   \
        ctx->tok.err = 0;              \
    }
#define CHANGE_TOK(x) ctx->tok.kind = (x)
#define HAS_CHARS (ctx->idx < ctx->buffer.len)

static Optional<TokenKind> get_reserved_word(String8 word);
static Token lex_next(Parser* ctx) {
    // NOTE here we assume we have it all in memory
    if(ctx->idx == ctx->buffer.len) {
    eof:
        // TODO: check for mismatched braces here
        ADD_TOK(TOK_EOF_);
        ctx->tok.n = 0;
        return ctx->tok;
    }
    for(;;) {
        switch(ctx->buffer[ctx->idx]) {
            case -1:
            case '\0':
                goto eof;
            case ' ':
            case '\t':
                CONSUME;
                break;
            // comments
            case '#': {
                char peek;
                do {
                    CONSUME;
                    peek = PEEK;
                } while(peek != '\n' && peek > 0);
                if(peek == '\n') CONSUME;
                continue;
            } break;
            case '\n':
                ADD_TOK(TOK_STATEMENT_END);
                CONSUME_NEXT_LINE;
                goto token_end;
                // if(ctx->state == LEX_STATE_IN_STATEMENT) {
                //     ADD_TOK(TOK_STATEMENT_END);
                //     CONSUME_NEXT_LINE;
                //     goto token_end;
                // }
                // CONSUME_NEXT_LINE;
                // continue;
            case ';':
                ADD_TOK(TOK_STATEMENT_END);
                CONSUME;
                goto token_end;
            case '+':
                ADD_TOK(TOK_PLUS_OP);
                if(PEEK == '=') {
                    CHANGE_TOK(TOK_PLUS_EQUALS_OP);
                    CONSUME_X(2);
                    goto token_end;
                } else {
                    CONSUME;
                    goto token_end;
                }
            case '-':
                ADD_TOK(TOK_MINUS_OP);
                if(PEEK == '=') {
                    CHANGE_TOK(TOK_MINUS_EQUALS_OP);
                    CONSUME_X(2);
                    goto token_end;
                } else {
                    CONSUME;
                    goto token_end;
                }
                goto token_end;
            case '*':
                ADD_TOK(TOK_MUL_OP);
                if(PEEK == '=') {
                    CHANGE_TOK(TOK_MUL_EQUALS_OP);
                    CONSUME_X(2);
                    goto token_end;
                } else {
                    CONSUME;
                    goto token_end;
                }
            case '/':
                ADD_TOK(TOK_DIV_OP);
                if(PEEK == '=') {
                    CHANGE_TOK(TOK_DIV_EQUALS_OP);
                    CONSUME_X(2);
                    goto token_end;
                } else {
                    CONSUME;
                    goto token_end;
                }
            case '&':
                ADD_TOK(TOK_AND_OP);
                if(PEEK == '=') {
                    CHANGE_TOK(TOK_AND_EQUALS_OP);
                    CONSUME_X(2);
                    goto token_end;
                } else {
                    CONSUME;
                    goto token_end;
                }
            case '|':
                ADD_TOK(TOK_OR_OP);
                if(PEEK == '=') {
                    CHANGE_TOK(TOK_OR_EQUALS_OP);
                    CONSUME_X(2);
                    goto token_end;
                } else {
                    CONSUME;
                    goto token_end;
                }
            case ',':
                ADD_TOK(TOK_COMMA);
                CONSUME;
                goto token_end;
            case '(':
                ADD_TOK(TOK_BRACKET_LEFT);
                CONSUME;
                goto token_end;
            case ')':
                ADD_TOK(TOK_BRACKET_RIGHT);
                CONSUME;
                goto token_end;
            case '[':
                ADD_TOK(TOK_INDEX_BRACKET_LEFT);
                CONSUME;
                goto token_end;
            case ']':
                ADD_TOK(TOK_INDEX_BRACKET_RIGHT);
                CONSUME;
                goto token_end;
            case '{':
                ADD_TOK(TOK_SCOPE_BRACKET_LEFT);
                CONSUME;
                goto token_end;
            case '}':
                ADD_TOK(TOK_SCOPE_BRACKET_RIGHT);
                CONSUME;
                goto token_end;
            case ':': {
                ADD_TOK(TOK_COLON_OP);
                if(PEEK == '=') {
                    CHANGE_TOK(TOK_COLON_EQUALS_OP);
                    CONSUME_X(2);
                    goto token_end;
                } else {
                    CONSUME;
                    goto token_end;
                }
            }
            case '.': {
                ADD_TOK(TOK_DOT_OP);
                switch(PEEK) {
                    case '.':
                        CHANGE_TOK(TOK_DOUBLE_DOT_OP);
                        CONSUME_X(2);
                        break;
                    default:
                        CONSUME;
                        break;
                }
                goto token_end;
            }
            case '>': {
                ADD_TOK(TOK_GREATER_THAN_OP);
                switch(PEEK) {
                    case '=':
                        CHANGE_TOK(TOK_GREATER_THAN_EQUAL_TO_OP);
                        CONSUME_X(2);
                        break;
                    default:
                        CONSUME;
                        break;
                }
                goto token_end;
            }
            case '<': {
                ADD_TOK(TOK_LESS_THAN_OP);
                switch(PEEK) {
                    case '=':
                        CHANGE_TOK(TOK_LESS_THAN_EQUAL_TO_OP);
                        CONSUME_X(2);
                        break;
                    default:
                        CONSUME;
                        break;
                }
                goto token_end;
            }
            case '=': {
                ADD_TOK(TOK_EQUALS_OP);
                switch(PEEK) {
                    case '>':
                        CHANGE_TOK(TOK_MAP_OP);
                        CONSUME_X(2);
                        break;
                    case '=':
                        CHANGE_TOK(TOK_EQUALITY_OP);
                        CONSUME_X(2);
                        break;
                    default:
                        CONSUME;
                        break;
                }
                goto token_end;
            }
            case DIGIT: {
                // TODO:
                // - handle alpha without space in-between
                ADD_TOK(TOK_INT_LITERAL);
                CONSUME;

                while(HAS_CHARS) {
                    const char ch = ctx->buffer[ctx->idx];
                    if(ch >= '0' && ch <= '9') {
                        CONSUME;
                    } else if((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
                        ctx->tok.err = 1;
                        break;
                    } else if(ch == '.') {
                        const char prev = ctx->buffer[ctx->idx - 1];
                        if(prev == '.') {
                            ctx->idx -= 1; // take back token (sequence needs to be re-tokenized)
                            break;
                        } else if(ctx->tok.kind == TOK_FLOAT_LITERAL) {
                            ctx->tok.err = 1;
                        }
                        ctx->tok.kind = TOK_FLOAT_LITERAL;
                        CONSUME;
                        continue;
                    } else {
                        break;
                    }
                }
                goto token_end;
            }
            case ALPHA: {
                ADD_TOK(TOK_IDENTIFIER);
                CONSUME;
                while(HAS_CHARS) {
                    const char ch = ctx->buffer[ctx->idx];
                    if((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
                        CONSUME;
                    } else {
                        break;
                    }
                }
                ctx->tok.n = ctx->idx - ctx->tok.idx;

                auto reserved_kw = get_reserved_word(ctx->tok.ss(ctx->buffer));
                if(reserved_kw.exists) {
                    ctx->tok.kind = reserved_kw.value;
                }
                return ctx->tok;
            }
            case '\'':
            case '"': {
                const char ch = ctx->buffer[ctx->idx];

                ADD_TOK(TOK_STRING_LITERAL);
                CONSUME;
                bool literal_ends = false;
                while(HAS_CHARS) {
                    CONSUME;
                    if(ctx->buffer[ctx->idx] == ch) {
                        literal_ends = true;
                        break;
                    }
                }

                if(!literal_ends) {
                    ctx->tok.err = 1;
                } else {
                    CONSUME;
                }
                goto token_end;
            }
        }
    }
token_end:
    ctx->tok.n = ctx->idx - ctx->tok.idx;
    return ctx->tok;
}

struct ReservedWord {
    String8 name;
    TokenKind kind;
};

ReservedWord RESERVED_WORDS[] = {
    {S8_LIT("return"), TOK_RETURN_KEYWORD},
    {S8_LIT("const"), TOK_CONST_KEYWORD},
    {S8_LIT("var"), TOK_VAR_KEYWORD},
    {S8_LIT("type"), TOK_TYPE_KEYWORD},
    {S8_LIT("struct"), TOK_STRUCT_KEYWORD},
    {S8_LIT("union"), TOK_UNION_KEYWORD},
    {S8_LIT("enum"), TOK_ENUM_KEYWORD},
    {S8_LIT("func"), TOK_FUNCTION_KEYWORD},
    {S8_LIT("defer"), TOK_DEFER_KEYWORD},
    {S8_LIT("import"), TOK_IMPORT_KEYWORD},
    {S8_LIT("while"), TOK_WHILE_KEYWORD},
    {S8_LIT("for"), TOK_FOR_KEYWORD},
    {S8_LIT("continue"), TOK_CONTINUE_KEYWORD},
    {S8_LIT("break"), TOK_BREAK_KEYWORD},
    {S8_LIT("static"), TOK_STATIC_KEYWORD},
    {S8_LIT("if"), TOK_IF_KEYWORD},
    {S8_LIT("elif"), TOK_ELIF_KEYWORD},
    {S8_LIT("else"), TOK_ELSE_KEYWORD},
    {S8_LIT("switch"), TOK_SWITCH_KEYWORD},
    {S8_LIT("case"), TOK_CASE_KEYWORD},
    {S8_LIT("true"), TOK_BOOL_LITERAL},
    {S8_LIT("false"), TOK_BOOL_LITERAL},
    {S8_LIT("nil"), TOK_NIL_LITERAL},
    {S8_LIT("not"), TOK_NOT_OP},
    {S8_LIT("and"), TOK_AND_OP},
    {S8_LIT("or"), TOK_OR_OP},
};

Optional<TokenKind> get_reserved_word(String8 word) {
    for(ReservedWord p : RESERVED_WORDS) {
        if(p.name == word) {
            return {p.kind, true};
        }
    }
    return {TOK_UNINTIALIZED, false};
}

static Optional<i128> atoi128(String8 str) {
    if(str.len == 0) {
        return {0, false};
    }

    i128 result = 0;
    bool valid = true;
    int sign = 1;
    for(size_t i = 0; i < str.len; ++i) {
        char ch = str.data[i];
        if(ch == '-') {
            sign = -1;
            if(i > 0) {
                valid = false;
                break;
            }
        } else if(ch == '+') {
            if(i > 0) {
                valid = false;
                break;
            }
        } else {
            result *= 10;
            if(ch > '9' || ch < '0') {
                valid = false;
                break;
            }
            result += (ch - '0');
        }
    }
    return {result * sign, valid};
}
