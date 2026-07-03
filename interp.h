#ifndef INTERP_H
#define INTERP_H

#include "ast.h"
#include "funcdef.h"

/* Registry entry for a function or procedure definition. */
typedef struct func_def {
    char*        name;
    char         kind;      /* FUNC_TYPE or PROC_TYPE from defs_min.h */
    param_info*  params;    /* ordered list of formal parameters */
    ASTNode*     body;      /* body sequence (declare nodes + statements) */
    struct func_def* next;
} func_def;

/* Global function/procedure registry (populated during parsing). */
extern func_def* func_registry;

/* Called from parser.y for each function/procedure definition. */
void register_function(char* name, char kind, param_info* params, ASTNode* body);

/* Look up a function/procedure by name. Returns NULL if not found. */
func_def* lookup_func(const char* name);

/* Interprets the program AST rooted at root. */
void interpret(ASTNode* root);

#endif /* INTERP_H */
