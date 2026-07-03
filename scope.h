#ifndef   __SCOPE_H
#define   __SCOPE_H
#include "sym_tab.h"

typedef struct strstack
{
	symbol_table st;
	struct strstack *next;
}*stack_st;

extern stack_st stack_sym_table;

void initstack_st(void);
void push_st(symbol_table table);
symbol_table pop_st(void);
symbol_table current_scope(void);

entry find_ID(char *name);
//������ �� ���� �� ID(name) ����� ��� ������ �����. �� ��� �� ����
//��������� ��� ������ ����������� ��� ����� ������ �.�.� 
//���������� null �� ��� ���� �������(��� �� ���� ����).

#endif