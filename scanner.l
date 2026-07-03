%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"

int line   = 1;
int column = 0;

void count(void);
void comment_block(void);
void comment_line(void);
%}

Letter     [a-zA-Z]
Digit      [0-9]
Identifier {Letter}({Letter}|{Digit})*
Number     {Digit}+
Ws         [ \t\v\f\r\n]+

%%

{Ws}            { count(); }

"/*"            { count(); comment_block(); }
"//"            { count(); comment_line();  }

"program"       { count(); return PROGRAM;     }
"declare"       { count(); return DECLARE;     }
"if"            { count(); return IF;          }
"else"          { count(); return ELSE;        }
"while"         { count(); return WHILE;       }
"doublewhile"   { count(); return DOUBLEWHILE; }
"loop"          { count(); return LOOP;        }
"exit"          { count(); return EXIT_TOK;    }
"forcase"       { count(); return FORCASE;     }
"incase"        { count(); return INCASE;      }
"when"          { count(); return WHEN;        }
"default"       { count(); return DEFAULT;     }
"not"           { count(); return NOT;         }
"and"           { count(); return AND;         }
"or"            { count(); return OR;          }
"function"      { count(); return FUNCTION;    }
"procedure"     { count(); return PROCEDURE;   }
"call"          { count(); return CALL;        }
"return"        { count(); return RETURN;      }
"inout"         { count(); return INOUT;       }
"input"         { count(); return INPUT;       }
"in"            { count(); return IN;          }
"print"         { count(); return PRINT;       }

{Identifier}    {
                    char name_buf[32];
                    count();
                    if (yyleng > 30) {
                        fprintf(stderr,
                            "Lexical error (line %d, col %d): "
                            "identifier '%s' exceeds 30 characters\n",
                            line, column, yytext);
                        strncpy(name_buf, yytext, 30);
                        name_buf[30] = '\0';
                    } else {
                        strcpy(name_buf, yytext);
                    }
                    yylval.name = (char *)malloc(strlen(name_buf) + 1);
                    if (!yylval.name) { fprintf(stderr, "out of memory\n"); exit(1); }
                    strcpy(yylval.name, name_buf);
                    return IDENTIFIER;
                }

{Number}        {
                    count();
                    long val = atol(yytext);
                    if (val > 65535) {
                        fprintf(stderr,
                            "Lexical error (line %d, col %d): "
                            "constant '%s' out of range (max 65535)\n",
                            line, column, yytext);
                        val = 65535;
                    }
                    yylval.value = (int)val;
                    return NUMBER;
                }

":="            { count(); return ASSIGN; }
"<="            { count(); return LE;     }
">="            { count(); return GE;     }
"<>"            { count(); return NEQ;    }
"+"             { count(); return '+'; }
"-"             { count(); return '-'; }
"*"             { count(); return '*'; }
"/"             { count(); return '/'; }
"<"             { count(); return '<'; }
">"             { count(); return '>'; }
"="             { count(); return '='; }
";"             { count(); return ';'; }
","             { count(); return ','; }
":"             { count(); return ':'; }
"("             { count(); return '('; }
")"             { count(); return ')'; }
"["             { count(); return '['; }
"]"             { count(); return ']'; }
"{"             { count(); return '{'; }
"}"             { count(); return '}'; }

.               {
                    count();
                    fprintf(stderr,
                        "Lexical error (line %d, col %d): "
                        "unrecognized character '%s'\n",
                        line, column, yytext);
                }

%%

int yywrap(void) { return 1; }

void comment_block(void){
    int c, c1;
loop:
    while ((c = input()) != '*' && c != 0) {
        // Know where you are after a multiline comment
        if (c == '\n') { line++; column = 0; }
        else column++;
    }
    if (c != 0 && (c1 = input()) != '/') {
        column++;               /* count the stray '*' */
        if (c1 != 0) unput(c1);
        goto loop;
    }
    if (c == 0) {
        fprintf(stderr,
            "Lexical error (line %d, col %d): unterminated block comment\n",
            line, column);
        /* do NOT unput — just return, let scanner handle EOF naturally */
    } else {
        column += 2; /* account for closing */
    }
}

void comment_line(void){
    int c;
    while ((c = input()) != '\n' && c != 0)
        column++;
    if (c == '\n') { line++; column = 0; }
}

void count(void){
    int i;
    for(i=0;yytext[i]!='\0';i++){
        if(yytext[i]=='\n'){
            line++;
            column=0;
        }
        else if(yytext[i]=='\t')
            column += 8-(column % 8);
        else column++;
    }                
}
