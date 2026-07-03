/* Put gener.h into the generated parser.tab.h so any file that
   includes parser.tab.h (e.g. lex.yy.c) also gets decl_list.    */
%code requires {
#include "gener.h"
#include "ast.h"      /* βάζουμε ΕΔΩ για να περάσει στο parser.tab.h */
#include "funcdef.h"  /* param_info — χρειάζεται στο %union */
}

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs_min.h"
#include "sym_tab.h"
#include "scope.h"
#include "gener.h"
#include "ast.h"
#include "interp.h"  /* register_function(), interpret() */

extern int line;
extern int column;

ASTNode* root = NULL; /* ΠΡΟΣΘΗΚΗ: Εδώ θα αποθηκευτεί η ρίζα του δέντρου */

static void add_to_scope(char *name, char type);
int yyerror(const char *msg);
%}

%union {
    char        *name;
    int          value;
    decl_list    idlist;
    ASTNode*     node;
    param_info*  params;   /* για τυπικές παραμέτρους συναρτήσεων */
}

%token <name>  IDENTIFIER
%token <value> NUMBER

%token PROGRAM DECLARE
%token IF ELSE WHILE DOUBLEWHILE LOOP EXIT_TOK
%token FORCASE INCASE WHEN DEFAULT
%token NOT AND OR
%token FUNCTION PROCEDURE CALL RETURN
%token IN INOUT INPUT PRINT
%token ASSIGN LE GE NEQ

%type <idlist> varlist

%type <node> declarations
%type <node> block brack_or_stat sequence statement assignment_stat if_stat elsepart
%type <node> while_stat doublewhile_stat loop_stat forcase_stat incase_stat
%type <node> when_list when_stat exit_stat return_stat print_stat input_stat call_stat
%type <node> condition boolterm boolfactor expression term factor actualpars actualparlist actualparitem idtail
%type <value> relop addop mulop optional_sign
%type <params> formalpars formalparlist formalparitem

%start program
%expect 2   /* 1: dangling-else (ELSE), 1: nested incase/forcase (WHEN) */

%%

/* ------------------------------------------------------------------ */
/* Top-level                                                          */
/* ------------------------------------------------------------------ */

program
    : PROGRAM IDENTIFIER block 
        { 
            root = $3; /* Αποθήκευση του τελικού δέντρου στη ρίζα */
        }
    ;

/* ------------------------------------------------------------------ */
/* Block — pushes its own scope                                       */
/* ------------------------------------------------------------------ */

block
    : '{' begin_scope declarations subprograms sequence end_scope '}'
        {
            /* $3 = declare nodes, $5 = statement sequence
               new_seq_node(NULL, x) == x, so no-op when no declarations */
            $$ = new_seq_node($3, $5);
        }
    ;

begin_scope
    : /* empty */ { push_st(create_st()); }
    ;

end_scope
    : /* empty */ { pop_st(); }
    ;

/* ------------------------------------------------------------------ */
/* Declarations                                                       */
/* ------------------------------------------------------------------ */

declarations
    : /* empty */ { $$ = NULL; }
    | declarations DECLARE varlist ';'
        {
            ASTNode* seq = $1;
            decl_list dl = $3;
            while (dl != NULL) {
                add_to_scope(dl->name, DECL_TYPE);
                seq = new_seq_node(seq, new_declare_node(dl->name, line));
                dl = dl->next;
            }
            $$ = seq;
        }
    ;

varlist
    : IDENTIFIER
        {
            $$ = create_dl();
            $$->name = $1;
            $$->next = NULL;
        }
    | varlist ',' IDENTIFIER
        {
            decl_list nd = create_dl();
            nd->name = $3;
            nd->next = $1;
            $$ = nd;
        }
    ;

/* ------------------------------------------------------------------ */
/* Subprograms                                                        */
/* ------------------------------------------------------------------ */

subprograms
    : /* empty */
    | subprograms func
    ;

func
    : FUNCTION IDENTIFIER
        {
            add_to_scope($2, FUNC_TYPE);
            push_st(create_st());
        }
      formalpars block
        {
            /* $4 = param list (param_info*), $5 = body (ASTNode*) */
            pop_st();
            register_function($2, FUNC_TYPE, $4, $5);
        }
    | PROCEDURE IDENTIFIER
        {
            add_to_scope($2, PROC_TYPE);
            push_st(create_st());
        }
      formalpars block
        {
            pop_st();
            register_function($2, PROC_TYPE, $4, $5);
        }
    ;

formalpars
    : '(' ')'               { $$ = NULL; }
    | '(' formalparlist ')' { $$ = $2;   }
    ;

formalparlist
    : formalparitem
        { $$ = $1; }
    | formalparlist ',' formalparitem
        {
            /* Append $3 to end of $1 to preserve declaration order */
            if ($1 == NULL) { $$ = $3; }
            else {
                param_info* tail = $1;
                while (tail->next) tail = tail->next;
                tail->next = $3;
                $$ = $1;
            }
        }
    ;

formalparitem
    : IN IDENTIFIER
        {
            param_info* p = (param_info*)malloc(sizeof(param_info));
            p->name = strdup($2); p->pass_mode = IN_PAR; p->next = NULL;
            add_to_scope($2, IN_PAR);
            $$ = p;
        }
    | INOUT IDENTIFIER
        {
            param_info* p = (param_info*)malloc(sizeof(param_info));
            p->name = strdup($2); p->pass_mode = INOUT_PAR; p->next = NULL;
            add_to_scope($2, INOUT_PAR);
            $$ = p;
        }
    ;

/* ------------------------------------------------------------------ */
/* Brack-or-stat and sequence                                         */
/* ------------------------------------------------------------------ */

brack_or_stat
    : '{' sequence '}' { $$ = $2; }
    | statement        { $$ = $1; }
    ;

sequence
    : statement                { $$ = $1; }
    | sequence ';' statement   { $$ = new_seq_node($1, $3); }
    ;

/* ------------------------------------------------------------------ */
/* Statements                                                         */
/* ------------------------------------------------------------------ */

statement
    : /* empty */          { $$ = NULL; }
    | assignment_stat      { $$ = $1; }
    | if_stat              { $$ = $1; }
    | while_stat           { $$ = $1; }
    | doublewhile_stat     { $$ = $1; }
    | loop_stat            { $$ = $1; }
    | forcase_stat         { $$ = $1; }
    | incase_stat          { $$ = $1; }
    | exit_stat            { $$ = $1; }
    | return_stat          { $$ = $1; }
    | print_stat           { $$ = $1; }
    | input_stat           { $$ = $1; }
    | call_stat            { $$ = $1; }
    ;

assignment_stat
    : IDENTIFIER ASSIGN expression
        {
            if (find_ID($1) == NULL) {
                char msg[256];
                snprintf(msg, sizeof(msg), "undeclared identifier '%s'", $1);
                yyerror(msg);
            }
            $$ = new_assign_node($1, $3, line);
        }
    ;

if_stat
    : IF '(' condition ')' brack_or_stat elsepart
        { $$ = new_if_node($3, $5, $6, line); }
    ;

elsepart
    : /* empty */          { $$ = NULL; }
    | ELSE brack_or_stat   { $$ = $2; }
    ;

while_stat
    : WHILE '(' condition ')' brack_or_stat
        { $$ = new_while_node($3, $5, line); }
    ;

doublewhile_stat
    : DOUBLEWHILE '(' condition ')' brack_or_stat ELSE brack_or_stat
        { $$ = new_doublewhile_node($3, $5, $7, line); }
    ;

loop_stat
    : LOOP brack_or_stat
        { $$ = new_loop_node($2, line); }
    ;

forcase_stat
    : FORCASE when_list DEFAULT ':' brack_or_stat
        { $$ = new_forcase_node($2, $5, line); }
    ;

incase_stat
    : INCASE when_list
        { $$ = new_incase_node($2, line); }
    ;

when_list
    : /* empty */           { $$ = NULL; }
    | when_list when_stat   { $$ = new_seq_node($1, $2); }
    ;

when_stat
    : WHEN ':' '(' condition ')' ':' brack_or_stat
        { $$ = new_when_node($4, $7, line); }
    ;

exit_stat
    : EXIT_TOK
        { $$ = new_simple_cmd_node(AST_EXIT, NULL, NULL, line); }
    ;

return_stat
    : RETURN '(' expression ')'
        { $$ = new_simple_cmd_node(AST_RETURN, $3, NULL, line); }
    ;

print_stat
    : PRINT '(' expression ')'
        { $$ = new_simple_cmd_node(AST_PRINT, $3, NULL, line); }
    ;

input_stat
    : INPUT '(' IDENTIFIER ')'
        {
            if (find_ID($3) == NULL) {
                char msg[256];
                snprintf(msg, sizeof(msg), "undeclared identifier '%s'", $3);
                yyerror(msg);
            }
            $$ = new_simple_cmd_node(AST_INPUT, NULL, $3, line);
        }
    ;

call_stat
    : CALL IDENTIFIER actualpars
        {
            if (find_ID($2) == NULL) {
                char msg[256];
                snprintf(msg, sizeof(msg), "undeclared identifier '%s'", $2);
                yyerror(msg);
            }
            $$ = new_call_node($2, $3, line);
        }
    ;

actualpars
    : '(' ')'                 { $$ = NULL; }
    | '(' actualparlist ')'   { $$ = $2; }
    ;

actualparlist
    : actualparitem                   { $$ = $1; }
    | actualparlist ',' actualparitem { $$ = new_seq_node($1, $3); }
    ;

actualparitem
    : IN    expression   { $$ = $2; }
    | INOUT IDENTIFIER
        {
            if (find_ID($2) == NULL) {
                char msg[256];
                snprintf(msg, sizeof(msg), "undeclared identifier '%s'", $2);
                yyerror(msg);
            }
            $$ = new_id_node($2, line);
        }
    ;

/* ------------------------------------------------------------------ */
/* Conditions                                                         */
/* ------------------------------------------------------------------ */

condition
    : boolterm                    { $$ = $1; }
    | condition OR boolterm       { $$ = new_binop_node(OR, $1, $3, line); }
    ;

boolterm
    : boolfactor                  { $$ = $1; }
    | boolterm AND boolfactor     { $$ = new_binop_node(AND, $1, $3, line); }
    ;

boolfactor
    : NOT '[' condition ']'           { $$ = new_unop_node(NOT, $3, line); }
    | '[' condition ']'               { $$ = $2; }
    | expression relop expression     { $$ = new_binop_node($2, $1, $3, line); }
    ;

relop
    : '='  { $$ = '='; } 
    | '<'  { $$ = '<'; } 
    | LE   { $$ = LE; } 
    | NEQ  { $$ = NEQ; } 
    | GE   { $$ = GE; } 
    | '>'  { $$ = '>'; }
    ;

/* ------------------------------------------------------------------ */
/* Expressions                                                        */
/* ------------------------------------------------------------------ */

expression
    : optional_sign term
        {
            if ($1 == 0) $$ = $2;
            else $$ = new_unop_node($1, $2, line);
        }
    | expression addop term
        { $$ = new_binop_node($2, $1, $3, line); }
    ;

term
    : factor                  { $$ = $1; }
    | term mulop factor       { $$ = new_binop_node($2, $1, $3, line); }
    ;

factor
    : NUMBER                  { $$ = new_num_node($1, line); }
    | '(' expression ')'      { $$ = $2; }
    | IDENTIFIER idtail
        {
            if (find_ID($1) == NULL) {
                char msg[256];
                snprintf(msg, sizeof(msg), "undeclared identifier '%s'", $1);
                yyerror(msg);
            }
            if ($2 == NULL) {
                $$ = new_id_node($1, line);
            } else {
                $$ = new_call_node($1, $2, line); /* Αναγνώριση κλήσης συνάρτησης */
            }
        }
    ;

idtail
    : /* empty */  { $$ = NULL; }
    | actualpars   { $$ = $1; }
    ;

addop : '+' { $$ = '+'; } | '-' { $$ = '-'; } ;
mulop : '*' { $$ = '*'; } | '/' { $$ = '/'; } ;

optional_sign
    : /* empty */ { $$ = 0; }
    | addop       { $$ = $1; }
    ;

%%

/* ------------------------------------------------------------------ */
/* Helper: insert an identifier into the current scope                */
/* ------------------------------------------------------------------ */
static void add_to_scope(char *name, char type)
{
    entry e = (entry)malloc(sizeof(struct strentry));
    if (!e) { fprintf(stderr, "out of memory\n"); exit(1); }
    e->name = name;   /* takes ownership of the scanner-malloc'd string */
    e->type = type;
    e->val  = 0;
    if (enter(current_scope(), e) == 0) {
        free(e);
        char msg[256];
        snprintf(msg, sizeof(msg), "duplicate declaration of '%s'", name);
        yyerror(msg);
    }
}

/* ------------------------------------------------------------------ */
/* Error reporting                                                    */
/* ------------------------------------------------------------------ */
int yyerror(const char *msg)
{
    fprintf(stderr, "Error (line %d, col %d): %s\n", line, column, msg);
    return 1;
}

/* ------------------------------------------------------------------ */
/* Λειτουργία Εκτύπωσης του Δέντρου (Για επαλήθευση)                  */
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
/* ΒΕΛΤΙΩΜΕΝΗ Λειτουργία Εκτύπωσης του Δέντρου                        */
/* ------------------------------------------------------------------ */
void print_ast(ASTNode* node, int level) {
    if (node == NULL) return;
    
    for (int i = 0; i < level; i++) printf("  "); /* Κενά για ιεραρχία */

    switch (node->type) {
        case AST_NUM: printf("Number: %d\n", node->int_val); break;
        case AST_ID: printf("Identifier: %s\n", node->id_name); break;
        case AST_ASSIGN: printf("Assignment (%s :=)\n", node->assign.id_name); break;
        case AST_IF: printf("If Statement\n"); break;
        case AST_WHILE: printf("While Loop\n"); break;
        case AST_DOUBLEWHILE: printf("Doublewhile Loop\n"); break;
        case AST_LOOP: printf("Loop (Infinite)\n"); break;
        case AST_FORCASE: printf("Forcase Statement\n"); break;
        case AST_INCASE: printf("Incase Statement\n"); break;
        case AST_WHEN: printf("When Clause\n"); break;
        case AST_EXIT: printf("Exit Statement\n"); break;
        case AST_PRINT: printf("Print Statement\n"); break;
        case AST_INPUT: printf("Input Statement\n"); break;
        case AST_RETURN: printf("Return Statement\n"); break;
        case AST_CALL: printf("Function Call (%s)\n", node->func_call.id_name); break;
        case AST_BINOP: printf("Binary Operation (op code: %d)\n", node->binop.op); break;
        case AST_UNOP: printf("Unary Operation\n"); break;
        case AST_SEQ: /* Block εντολών */ break;
        default: printf("Node Type: %d\n", node->type); break;
    }

    /* Αναδρομικές κλήσεις για τα "παιδιά" του κόμβου */
    if (node->type == AST_SEQ) {
        print_ast(node->seq.first, level);
        print_ast(node->seq.next, level);
    } else if (node->type == AST_ASSIGN) {
        print_ast(node->assign.expr, level + 1);
    } else if (node->type == AST_IF) {
        print_ast(node->if_stmt.cond, level + 1);
        print_ast(node->if_stmt.if_branch, level + 1);
        print_ast(node->if_stmt.else_branch, level + 1);
    } else if (node->type == AST_WHILE) {
        print_ast(node->loop_stmt.cond, level + 1);
        print_ast(node->loop_stmt.body, level + 1);
    } else if (node->type == AST_DOUBLEWHILE) {
        print_ast(node->doublewhile_stmt.cond, level + 1);
        print_ast(node->doublewhile_stmt.true_body, level + 1);
        print_ast(node->doublewhile_stmt.false_body, level + 1);
    } else if (node->type == AST_LOOP) {
        print_ast(node->loop_stmt.body, level + 1);
    } else if (node->type == AST_FORCASE) {
        print_ast(node->forcase_stmt.when_list, level + 1);
        print_ast(node->forcase_stmt.def_body, level + 1);
    } else if (node->type == AST_INCASE) {
        print_ast(node->incase_stmt.when_list, level + 1);
    } else if (node->type == AST_WHEN) {
        print_ast(node->when_stmt.cond, level + 1);
        print_ast(node->when_stmt.body, level + 1);
    } else if (node->type == AST_BINOP) {
        print_ast(node->binop.left, level + 1);
        print_ast(node->binop.right, level + 1);
    } else if (node->type == AST_UNOP) {
        print_ast(node->unop.expr, level + 1);
    } else if (node->type == AST_CALL || node->type == AST_PRINT || node->type == AST_RETURN) {
        print_ast(node->func_call.expr_or_pars, level + 1);
    }
}
/* ------------------------------------------------------------------ */
/* Entry point                                                        */
/* ------------------------------------------------------------------ */
/* ΠΡΟΣΘΗΚΗ: Δήλωση της μεταβλητής αρχείου του Flex */
extern FILE *yyin;

int main(int argc, char **argv)
{
    /* Έλεγχος αν δόθηκε αρχείο ως όρισμα (π.χ. ./minicc test.min) */
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            fprintf(stderr, "Σφάλμα: Δεν ήταν δυνατό το άνοιγμα του αρχείου %s\n", argv[1]);
            return 1;
        }
    }

    initstack_st();
    push_st(create_st());   /* global scope */

    int result = yyparse();

    if (result == 0 && root != NULL) {
        interpret(root);
    } else {
        fprintf(stderr, "Σφάλμα κατά την ανάλυση.\n");
    }

    /* Κλείσιμο του αρχείου αν ανοίχτηκε */
    if (yyin) {
        fclose(yyin);
    }

    return result;
}