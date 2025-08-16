#include "examples/parser.h"

#include <map> // TODO: removeme
#include <print>
#include <stdio.h>

struct Interpreter {
    std::map<std::string, AstNode*> funcs;
};

int dfs(Interpreter* ctx, Module* mod, AstNode* node) {
    switch(node->kind) {
        case NODE_MODULE: {
            for(size_t i = 0; i < node->kids.len; ++i) {
                int tmp = dfs(ctx, mod, node->kids[i]);
                std::println("node[{}] = {}", i, tmp);
                if(i == node->kids.len - 1) {
                    return tmp;
                }
            }
        } break;
        case NODE_FUNC_DECL: {
            // TODO
        } break;
        case NODE_FUNC_CALL: {
            // TODO
        } break;
        case NODE_IF: {
        } break;
        case NODE_BIN_OP: {
            auto lhs = dfs(ctx, mod, node->kids[0]);
            auto rhs = dfs(ctx, mod, node->kids[1]);
            switch(node->tok.kind) {
                case TOK_DIV_OP:
                    return lhs / rhs;
                case TOK_MUL_OP:
                    return lhs * rhs;
                case TOK_PLUS_OP:
                    return lhs + rhs;
                case TOK_MINUS_OP:
                    return lhs - rhs;
                default:
                    std::println("invalid bin op: {}", (int) node->tok.kind);
                    break;
            }
        }
        case NODE_BOOL_LIT: {
            return node->data.numeral_literal.value;
        } break;
        case NODE_INT_LIT: {
            return node->data.numeral_literal.value;
        } break;
        case NODE_ELIF: {
            std::println("unexpected elif");
            exit(1);
        }
        case NODE_ELSE: {
            std::println("unexpected else");
            exit(1);
        }
        default: {
            std::println("invalid node kind: {}", (int) node->kind);
        } break;
    }
}

int eval(Interpreter* ctx, Module* mod) {
    return dfs(ctx, mod, mod->root);
}

int main(int argc, char* argv[]) {
    if(argc == 1) {
        fprintf(stderr, "expected input file, usage:\n%s <input-file>\n", argv[0]);
        return 1;
    }
    Module* mod = module_make(S8_LIT("main"), nullptr, nullptr);

    ParseFileResult parse_result = module_parse_file(mod, S8_CSTR(argv[1]));
    if(parse_result) {
        fprintf(stderr, "Failed to parse: %s\n", argv[1]);
        // TODO
        fprintf(stderr, "Reason: %lld\n", (i64) parse_result.file_err);
        fflush(stderr);
        return 2;
    }

    Interpreter interpreter = {};
    int result = eval(&interpreter, mod);
    fprintf(stdout, "%d\n", result);
    return 0;
}
