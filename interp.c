#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"   /* token values: AND, OR, NOT, LE, GE, NEQ */
#include "interp.h"
#include "sym_tab.h"
#include "scope.h"
#include "defs_min.h"

/* ------------------------------------------------------------------ */
/* Function registry                                                   */
/* ------------------------------------------------------------------ */

func_def* func_registry = NULL;

void register_function(char* name, char kind, param_info* params, ASTNode* body) {
    func_def* fd = (func_def*)malloc(sizeof(func_def));
    fd->name   = strdup(name);
    fd->kind   = kind;
    fd->params = params;
    fd->body   = body;
    fd->next   = func_registry;
    func_registry = fd;
}

func_def* lookup_func(const char* name) {
    func_def* fd = func_registry;
    while (fd) {
        if (strcmp(fd->name, name) == 0) return fd;
        fd = fd->next;
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Signal mechanism for exit / return                                  */
/* ------------------------------------------------------------------ */

typedef enum { SIG_NONE, SIG_EXIT, SIG_RETURN } Signal;
static Signal cur_sig = SIG_NONE;
static int    ret_val = 0;

/* ------------------------------------------------------------------ */
/* Forward declarations                                                */
/* ------------------------------------------------------------------ */

static int  eval_expr(ASTNode* node);
static void exec_stmt(ASTNode* node);
static int  call_subprogram(const char* name, ASTNode* actuals);

/* ------------------------------------------------------------------ */
/* Runtime variable declaration                                        */
/* ------------------------------------------------------------------ */

static void rt_declare(const char* name) {
    entry e = (entry)malloc(sizeof(struct strentry));
    e->name = strdup(name);
    e->type = DECL_TYPE;
    e->val  = 0;
    if (enter(current_scope(), e) == 0) {
        /* Already declared in this scope (can happen on re-call): keep old entry */
        free(e->name);
        free(e);
    }
}

/* ------------------------------------------------------------------ */
/* Flatten left-recursive SEQ into an array (in declaration order)    */
/*                                                                     */
/* new_seq_node builds: SEQ(SEQ(n1,n2),n3) for n1,n2,n3               */
/* This helper collects [n1, n2, n3] into arr[].                       */
/* ------------------------------------------------------------------ */

static int collect_seq(ASTNode* node, ASTNode** arr, int max) {
    if (!node) return 0;
    if (node->type != AST_SEQ) {
        if (max < 1) return 0;
        arr[0] = node;
        return 1;
    }
    int n = collect_seq(node->seq.first, arr, max);
    if (n < max && node->seq.next)
        arr[n++] = node->seq.next;
    return n;
}

/* ------------------------------------------------------------------ */
/* Expression evaluator — returns integer value                        */
/* ------------------------------------------------------------------ */

static int eval_expr(ASTNode* node) {
    if (!node) return 0;

    switch (node->type) {

        case AST_NUM:
            return node->int_val;

        case AST_ID: {
            entry e = find_ID(node->id_name);
            if (!e) {
                fprintf(stderr, "Runtime error: undefined variable '%s'\n", node->id_name);
                return 0;
            }
            return e->val;
        }

        case AST_BINOP: {
            int L = eval_expr(node->binop.left);
            int R = eval_expr(node->binop.right);
            switch (node->binop.op) {
                case '+':  return L + R;
                case '-':  return L - R;
                case '*':  return L * R;
                case '/':
                    if (R == 0) {
                        fprintf(stderr, "Runtime error: division by zero\n");
                        return 0;
                    }
                    return L / R;
                case '=':  return (L == R) ? 1 : 0;
                case '<':  return (L <  R) ? 1 : 0;
                case '>':  return (L >  R) ? 1 : 0;
                case LE:   return (L <= R) ? 1 : 0;
                case GE:   return (L >= R) ? 1 : 0;
                case NEQ:  return (L != R) ? 1 : 0;
                case AND:  return (L && R) ? 1 : 0;
                case OR:   return (L || R) ? 1 : 0;
                default:
                    fprintf(stderr, "Runtime error: unknown binary operator %d\n", node->binop.op);
                    return 0;
            }
        }

        case AST_UNOP: {
            int V = eval_expr(node->unop.expr);
            switch (node->unop.op) {
                case '-':  return -V;
                case '+':  return  V;
                case NOT:  return (!V) ? 1 : 0;
                default:
                    fprintf(stderr, "Runtime error: unknown unary operator %d\n", node->unop.op);
                    return 0;
            }
        }

        case AST_CALL:
            return call_subprogram(node->func_call.id_name, node->func_call.expr_or_pars);

        default:
            fprintf(stderr, "Runtime error: unexpected node type %d in expression\n", node->type);
            return 0;
    }
}

/* ------------------------------------------------------------------ */
/* Statement executor                                                  */
/* ------------------------------------------------------------------ */

static void exec_stmt(ASTNode* node) {
    if (!node) return;
    if (cur_sig != SIG_NONE) return;   /* propagate exit/return signal */

    switch (node->type) {

        /* ---- Sequential composition ---- */
        case AST_SEQ:
            exec_stmt(node->seq.first);
            if (cur_sig) return;
            exec_stmt(node->seq.next);
            break;

        /* ---- Runtime variable declaration ---- */
        case AST_DECLARE_VAR:
            rt_declare(node->id_name);
            break;

        /* ---- Assignment ---- */
        case AST_ASSIGN: {
            int val = eval_expr(node->assign.expr);
            entry e = find_ID(node->assign.id_name);
            if (!e)
                fprintf(stderr, "Runtime error: undefined variable '%s'\n", node->assign.id_name);
            else
                e->val = val;
            break;
        }

        /* ---- if / else ---- */
        case AST_IF:
            if (eval_expr(node->if_stmt.cond))
                exec_stmt(node->if_stmt.if_branch);
            else if (node->if_stmt.else_branch)
                exec_stmt(node->if_stmt.else_branch);
            break;

        /* ---- while ---- */
        case AST_WHILE:
            while (!cur_sig && eval_expr(node->loop_stmt.cond))
                exec_stmt(node->loop_stmt.body);
            break;

        /* ---- doublewhile (cond) body1 else body2 ----
           Semantics: if cond initially true → loop body1 while cond
                      if cond initially false → execute body2 once   */
        case AST_DOUBLEWHILE:
            if (eval_expr(node->doublewhile_stmt.cond)) {
                while (!cur_sig && eval_expr(node->doublewhile_stmt.cond))
                    exec_stmt(node->doublewhile_stmt.true_body);
            } else {
                exec_stmt(node->doublewhile_stmt.false_body);
            }
            break;

        /* ---- loop (infinite, broken by exit) ---- */
        case AST_LOOP:
            while (1) {
                exec_stmt(node->loop_stmt.body);
                if (cur_sig == SIG_EXIT) { cur_sig = SIG_NONE; break; }
                if (cur_sig) break;   /* propagate SIG_RETURN */
            }
            break;

        /* ---- forcase: execute first matching when clause, else default ---- */
        case AST_FORCASE: {
            ASTNode* whens[64];
            int n = collect_seq(node->forcase_stmt.when_list, whens, 64);
            int matched = 0;
            for (int i = 0; i < n && !cur_sig; i++) {
                ASTNode* w = whens[i];
                if (w && w->type == AST_WHEN && eval_expr(w->when_stmt.cond)) {
                    exec_stmt(w->when_stmt.body);
                    matched = 1;
                    break;
                }
            }
            if (!matched && !cur_sig)
                exec_stmt(node->forcase_stmt.def_body);
            break;
        }

        /* ---- incase: execute ALL matching when clauses ---- */
        case AST_INCASE: {
            ASTNode* whens[64];
            int n = collect_seq(node->incase_stmt.when_list, whens, 64);
            for (int i = 0; i < n && !cur_sig; i++) {
                ASTNode* w = whens[i];
                if (w && w->type == AST_WHEN && eval_expr(w->when_stmt.cond))
                    exec_stmt(w->when_stmt.body);
            }
            break;
        }

        /* ---- exit (breaks out of loop) ---- */
        case AST_EXIT:
            cur_sig = SIG_EXIT;
            break;

        /* ---- return(expr) ---- */
        case AST_RETURN:
            ret_val = eval_expr(node->func_call.expr_or_pars);
            cur_sig = SIG_RETURN;
            break;

        /* ---- print(expr) ---- */
        case AST_PRINT:
            printf("%d\n", eval_expr(node->func_call.expr_or_pars));
            break;

        /* ---- input(id) ---- */
        case AST_INPUT: {
            entry e = find_ID(node->func_call.id_name);
            if (!e)
                fprintf(stderr, "Runtime error: undefined variable '%s'\n", node->func_call.id_name);
            else
                scanf("%d", &e->val);
            break;
        }

        /* ---- call proc(...) ---- */
        case AST_CALL:
            call_subprogram(node->func_call.id_name, node->func_call.expr_or_pars);
            break;

        default:
            /* AST_WHEN nodes are only accessed via collect_seq, never directly */
            break;
    }
}

/* ------------------------------------------------------------------ */
/* Function / Procedure call                                           */
/* ------------------------------------------------------------------ */

static int call_subprogram(const char* name, ASTNode* actuals) {
    func_def* fd = lookup_func(name);
    if (!fd) {
        fprintf(stderr, "Runtime error: undefined function/procedure '%s'\n", name);
        return 0;
    }

    /* Collect actual param nodes in declaration order */
    ASTNode* actual_arr[64];
    int n_actuals = collect_seq(actuals, actual_arr, 64);

    /* Pre-evaluate all actuals in the CALLER's scope before pushing new scope.
       For INOUT params the actual must be AST_ID (enforced by grammar). */
    int    actual_vals[64];
    entry  inout_callers[64];   /* caller entry to write back; NULL for IN params */

    param_info* fp = fd->params;
    for (int i = 0; fp && i < n_actuals; i++, fp = fp->next) {
        if (fp->pass_mode == IN_PAR) {
            actual_vals[i]    = eval_expr(actual_arr[i]);
            inout_callers[i]  = NULL;
        } else {  /* INOUT_PAR */
            entry caller_e    = find_ID(actual_arr[i]->id_name);
            actual_vals[i]    = caller_e ? caller_e->val : 0;
            inout_callers[i]  = caller_e;
        }
    }

    /* Push a fresh scope for this activation record */
    push_st(create_st());

    /* Bind formal parameters to their actual values */
    fp = fd->params;
    for (int i = 0; fp && i < n_actuals; i++, fp = fp->next) {
        entry e = (entry)malloc(sizeof(struct strentry));
        e->name = strdup(fp->name);
        e->type = fp->pass_mode;
        e->val  = actual_vals[i];
        enter(current_scope(), e);
    }

    /* Execute the function body */
    exec_stmt(fd->body);

    /* Capture return value (if any) and clear signal */
    int result = 0;
    if (cur_sig == SIG_RETURN) {
        result  = ret_val;
        cur_sig = SIG_NONE;
    }

    /* Write back INOUT formal values to their caller variables (before pop!) */
    fp = fd->params;
    for (int i = 0; fp && i < n_actuals; i++, fp = fp->next) {
        if (fp->pass_mode == INOUT_PAR && inout_callers[i]) {
            entry formal = lookup(current_scope(), fp->name);
            if (formal)
                inout_callers[i]->val = formal->val;
        }
    }

    pop_st();
    return result;
}

/* ------------------------------------------------------------------ */
/* Interpreter entry point                                             */
/* ------------------------------------------------------------------ */

void interpret(ASTNode* root) {
    /* Reset scope stack: parse-time entries are no longer needed.
       The runtime creates fresh entries for every declared variable. */
    initstack_st();
    push_st(create_st());   /* global runtime scope */

    cur_sig = SIG_NONE;
    ret_val = 0;

    exec_stmt(root);

    pop_st();
}
