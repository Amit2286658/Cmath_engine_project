/*
 * parser.h — Standalone Pratt Parser & AST Builder Module
 *
 * Purpose:
 *   Consumes tokens from `Lexer` and builds compact 8-byte `AstNode` trees in
 *   `MemArena` using operator binding powers, lexical `{ ... }` scopes,
 *   `if ... then ... else` chains, `switch` blocks, `cond ? x : y` ternary
 *   expressions, and `Tag : { ... }` labeled scopes from `SymTable`.
 */

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PARSER_MAX_LOCALS 32

typedef struct {
    StrSlice name;
    uint16_t type_id;
    uint8_t  slot;
    uint8_t  scope_depth;
} LocalVar;

typedef struct {
    StrSlice name;
    uint16_t body_ast;
    bool     defined;
} LabelDef;

typedef struct {
    Lexer      lex;
    bool       in_ext_body;
    LocalVar   locals[PARSER_MAX_LOCALS];
    uint16_t   slot_types[PARSER_MAX_LOCALS];
    uint8_t    vis_count;
    uint8_t    local_count;
    uint8_t    param_count;
    uint8_t    scope_depth;
    uint8_t    in_ternary_then;
    bool       last_was_block;
    LabelDef   labels[MEM_MAX_LABELS];
    uint8_t    label_count;
} Parser;

void     parser_init(Parser *P, MemArena *mem, const SymTable *tab,
                     const char *src, uint16_t len, uint16_t start_pos,
                     bool in_ext_body, bool stop_at_newline);

int      parser_add_local(Parser *P, const char *name, uint16_t len, uint16_t type_id);
uint16_t parser_parse_expr(Parser *P, int min_prec);
bool     parser_check_labels(Parser *P);

#ifdef __cplusplus
}
#endif

#endif /* PARSER_H */