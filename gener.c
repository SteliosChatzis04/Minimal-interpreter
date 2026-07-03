#include <stdio.h>
#include <malloc.h>
#include "gener.h"

decl_list create_dl(void)
{
	decl_list newid;
	newid=(decl_list)malloc(sizeof(struct strdecllist));
	newid->name=NULL;
	newid->next=NULL;
	return newid;
}
