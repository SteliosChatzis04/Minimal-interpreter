#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ast.h"

ASTNode* allocate_node(NodeType type, int line) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) { fprintf(stderr, "AST: Out of memory\n"); exit(1); }
    node->type = type;
    node->line = line;
    return node;
}

ASTNode* new_num_node(int val, int line) {
    ASTNode* n = allocate_node(AST_NUM, line);
    n->int_val = val; return n;
}

ASTNode* new_id_node(char* name, int line) {
    ASTNode* n = allocate_node(AST_ID, line);
    n->id_name = strdup(name); return n;
}

ASTNode* new_binop_node(int op, ASTNode* left, ASTNode* right, int line) {
    ASTNode* n = allocate_node(AST_BINOP, line);
    n->binop.op = op; n->binop.left = left; n->binop.right = right; return n;
}

ASTNode* new_unop_node(int op, ASTNode* expr, int line) {
    ASTNode* n = allocate_node(AST_UNOP, line);
    n->unop.op = op; n->unop.expr = expr; return n;
}

ASTNode* new_assign_node(char* name, ASTNode* expr, int line) {
    ASTNode* n = allocate_node(AST_ASSIGN, line);
    n->assign.id_name = strdup(name); n->assign.expr = expr; return n;
}

ASTNode* new_if_node(ASTNode* cond, ASTNode* if_b, ASTNode* else_b, int line) {
    ASTNode* n = allocate_node(AST_IF, line);
    n->if_stmt.cond = cond; n->if_stmt.if_branch = if_b; n->if_stmt.else_branch = else_b; return n;
}

ASTNode* new_while_node(ASTNode* cond, ASTNode* body, int line) {
    ASTNode* n = allocate_node(AST_WHILE, line);
    n->loop_stmt.cond = cond; n->loop_stmt.body = body; return n;
}

ASTNode* new_doublewhile_node(ASTNode* cond, ASTNode* true_b, ASTNode* false_b, int line) {
    ASTNode* n = allocate_node(AST_DOUBLEWHILE, line);
    n->doublewhile_stmt.cond = cond; n->doublewhile_stmt.true_body = true_b; n->doublewhile_stmt.false_body = false_b; return n;
}

ASTNode* new_loop_node(ASTNode* body, int line) {
    ASTNode* n = allocate_node(AST_LOOP, line);
    n->loop_stmt.cond = NULL; n->loop_stmt.body = body; return n;
}

ASTNode* new_when_node(ASTNode* cond, ASTNode* body, int line) {
    ASTNode* n = allocate_node(AST_WHEN, line);
    n->when_stmt.cond = cond; n->when_stmt.body = body; return n;
}

ASTNode* new_forcase_node(ASTNode* when_list, ASTNode* def_body, int line) {
    ASTNode* n = allocate_node(AST_FORCASE, line);
    n->forcase_stmt.when_list = when_list; n->forcase_stmt.def_body = def_body; return n;
}

ASTNode* new_incase_node(ASTNode* when_list, int line) {
    ASTNode* n = allocate_node(AST_INCASE, line);
    n->incase_stmt.when_list = when_list; return n;
}

ASTNode* new_call_node(char* name, ASTNode* pars, int line) {
    ASTNode* n = allocate_node(AST_CALL, line);
    n->func_call.id_name = strdup(name); n->func_call.expr_or_pars = pars; return n;
}

ASTNode* new_simple_cmd_node(NodeType t, ASTNode* expr, char* name, int line) {
    ASTNode* n = allocate_node(t, line);
    if(name) n->func_call.id_name = strdup(name);
    n->func_call.expr_or_pars = expr; return n;
}

ASTNode* new_seq_node(ASTNode* first, ASTNode* next) {
    if (!first) return next;
    ASTNode* n = allocate_node(AST_SEQ, first->line);
    n->seq.first = first; n->seq.next = next; return n;
}

ASTNode* new_declare_node(char* name, int line) {
    ASTNode* n = allocate_node(AST_DECLARE_VAR, line);
    n->id_name = strdup(name); return n;
}