/*
 * parser.h — Standalone Pratt Parser & AST Builder Module
 *
 * Purpose:
 *   Consumes tokens from `Lexer` and builds compact 8-byte `AstNode` trees in
 *   `MemArena` using operator binding powers and fixities from `SymTable`.
 */

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PARSER_MAX_LOCALS 16

typedef struct {
    StrSlice name;
    uint16_t type_id;
} LocalVar;

typedef struct {
    Lexer      lex;
    bool       in_ext_body;
    LocalVar   locals[PARSER_MAX_LOCALS];
    uint8_t    local_count;
    uint8_t    param_count;
} Parser;

void     parser_init(Parser *P, MemArena *mem, const SymTable *tab,
                     const char *src, uint16_t len, uint16_t start_pos,
                     bool in_ext_body, bool stop_at_newline);

int      parser_add_local(Parser *P, const char *name, uint16_t len, uint16_t type_id);
uint16_t parser_parse_expr(Parser *P, int min_prec);

#ifdef __cplusplus
}
#endif

#endif /* PARSER_H */