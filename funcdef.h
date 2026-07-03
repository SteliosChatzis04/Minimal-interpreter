#ifndef FUNCDEF_H
#define FUNCDEF_H

/* Parameter descriptor for a single formal parameter.
   Used in both parser.y (via %union) and interp.c. */
typedef struct param_info {
    char*  name;
    char   pass_mode;   /* IN_PAR or INOUT_PAR from defs_min.h */
    struct param_info* next;
} param_info;

#endif /* FUNCDEF_H */
