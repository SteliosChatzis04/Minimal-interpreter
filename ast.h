#ifndef AST_H
#define AST_H

typedef enum {
    AST_PROGRAM, AST_NUM, AST_ID,
    AST_BINOP, AST_UNOP,
    AST_ASSIGN, AST_IF, AST_WHILE, AST_DOUBLEWHILE,
    AST_LOOP, AST_FORCASE, AST_INCASE, AST_WHEN,
    AST_EXIT, AST_RETURN, AST_PRINT, AST_INPUT, AST_CALL,
    AST_SEQ,          /* Για λίστα εντολών (sequence) */
    AST_ACTUAL_PARS,  /* Για παραμέτρους κλήσης */
    AST_DECLARE_VAR   /* Δήλωση μεταβλητής — εκτελείται στον διερμηνευτή */
} NodeType;

typedef struct ASTNode {
    NodeType type;
    int line;

    union {
        int int_val;      /* Για σταθερές (CONSTANT) [cite: 77] */
        char* id_name;    /* Για μεταβλητές (ID) */
        
        /* Δυαδικές πράξεις (+, -, *, /, and, or, =, <, > κλπ) [cite: 78-80] */
        struct {
            int op; 
            struct ASTNode* left;
            struct ASTNode* right;
        } binop;

        /* Μοναδιαίες πράξεις (not, unary +, unary -) [cite: 76-77] */
        struct {
            int op;
            struct ASTNode* expr;
        } unop;

        /* Ανάθεση (ID := EXPR) [cite: 74] */
        struct {
            char* id_name;
            struct ASTNode* expr;
        } assign;

        /* If-Statement [cite: 74] */
        struct {
            struct ASTNode* cond;
            struct ASTNode* if_branch;
            struct ASTNode* else_branch; /* Μπορεί να είναι NULL */
        } if_stmt;

        /* While & Loop [cite: 75] */
        struct {
            struct ASTNode* cond; /* NULL για το Loop */
            struct ASTNode* body;
        } loop_stmt;

        /* Doublewhile (Συγκεκριμένο της minimal++) [cite: 75] */
        struct {
            struct ASTNode* cond;
            struct ASTNode* true_body;
            struct ASTNode* false_body;
        } doublewhile_stmt;

        /* When node (για forcase / incase) [cite: 75] */
        struct {
            struct ASTNode* cond;
            struct ASTNode* body;
        } when_stmt;

        /* Forcase [cite: 75] */
        struct {
            struct ASTNode* when_list; /* Συνδεδεμένη λίστα από when_stmts */
            struct ASTNode* def_body;  /* Το default block */
        } forcase_stmt;

        /* Incase [cite: 75] */
        struct {
            struct ASTNode* when_list;
        } incase_stmt;

        /* Call, Print, Return [cite: 75] */
        struct {
            char* id_name; /* Μόνο για Call / Input */
            struct ASTNode* expr_or_pars; 
        } func_call;

        /* Sequence (Λίστα κόμβων, π.χ. εντολές μέσα σε {}) [cite: 72] */
        struct {
            struct ASTNode* first;
            struct ASTNode* next;
        } seq;
    };
} ASTNode;

/* Δηλώσεις συναρτήσεων */
ASTNode* new_num_node(int val, int line);
ASTNode* new_id_node(char* name, int line);
ASTNode* new_binop_node(int op, ASTNode* left, ASTNode* right, int line);
ASTNode* new_unop_node(int op, ASTNode* expr, int line);
ASTNode* new_assign_node(char* name, ASTNode* expr, int line);
ASTNode* new_if_node(ASTNode* cond, ASTNode* if_branch, ASTNode* else_branch, int line);
ASTNode* new_while_node(ASTNode* cond, ASTNode* body, int line);
ASTNode* new_doublewhile_node(ASTNode* cond, ASTNode* true_body, ASTNode* false_body, int line);
ASTNode* new_loop_node(ASTNode* body, int line);
ASTNode* new_when_node(ASTNode* cond, ASTNode* body, int line);
ASTNode* new_forcase_node(ASTNode* when_list, ASTNode* def_body, int line);
ASTNode* new_incase_node(ASTNode* when_list, int line);
ASTNode* new_call_node(char* name, ASTNode* pars, int line);
ASTNode* new_simple_cmd_node(NodeType t, ASTNode* expr, char* name, int line);
ASTNode* new_seq_node(ASTNode* first, ASTNode* next);
ASTNode* new_declare_node(char* name, int line);

#endif