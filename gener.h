#ifndef   __GENER_H
#define   __GENER_H

//list of id's
//in yacc <declarator_list>

typedef struct strdecllist
{
	char *name;
	struct strdecllist *next;
}*decl_list;

decl_list create_dl(void);

#endif