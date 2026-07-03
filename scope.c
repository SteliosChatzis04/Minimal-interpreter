#include <stdio.h>
#include <malloc.h>
#include "sym_tab.h"
#include "scope.h"

/* Definition of the global scope stack (declared extern in scope.h) */
stack_st stack_sym_table;

void initstack_st(void)
{
	stack_sym_table=NULL;
}

void push_st(symbol_table table)
{
	if (stack_sym_table==NULL){
		stack_st newstackel;
		newstackel=(stack_st)malloc(sizeof(struct strstack));
		newstackel->st=table;
		newstackel->next=NULL;
		stack_sym_table=newstackel;
	}
	else {
		stack_st newstackel;
		newstackel=(stack_st)malloc(sizeof(struct strstack));
		newstackel->st=table;
		newstackel->next=stack_sym_table;
		stack_sym_table=newstackel;
	}
}

symbol_table pop_st(void)
{
	stack_st stackel;
	if (stack_sym_table!=NULL){
		stackel=stack_sym_table;
		stack_sym_table=stack_sym_table->next;
		return stackel->st;
	}
	else return NULL;
}

symbol_table current_scope(void)
{
	if (stack_sym_table!=NULL) return stack_sym_table->st;
	else return NULL;
}

entry find_ID(char *name)
{
	stack_st tmpptr=stack_sym_table;
	entry obt_entry=NULL;
	while ((stack_sym_table!=NULL)&&((obt_entry=lookup(current_scope(),name))==NULL))
			stack_sym_table=stack_sym_table->next;
	stack_sym_table=tmpptr;
	return obt_entry;
}
