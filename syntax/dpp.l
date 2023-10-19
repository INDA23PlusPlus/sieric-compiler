 /* vim: noet sw=8 ts=8
  */
%{
#include <stdio.h>
#include <stddef.h>
#include "y.tab.h"

void count(void);
%}

DIGIT	[0-9]
ID	[a-zA-Z_][a-zA-Z0-9_]*

%%

"let"		{ count(); return LET; }
"if"		{ count(); return IF; }
"else"		{ count(); return ELSE; }
"fn"		{ count(); return FN; }
"return"	{ count(); return RETURN; }

{ID}		{ count(); return IDENTIFIER; }
{DIGIT}+	{ count(); return CONSTANT; }

"&&"		{ count(); return LAND_OP; }
"||"		{ count(); return LOR_OP; }
"<="		{ count(); return LEQ_OP; }
">="		{ count(); return GEQ_OP; }
"<"		{ count(); return '<'; }
">"		{ count(); return '>'; }
"=="		{ count(); return EQ_OP; }
"!="		{ count(); return NEQ_OP; }
";"		{ count(); return ';'; }
"{"		{ count(); return '{'; }
"}"		{ count(); return '}'; }
"("		{ count(); return '('; }
")"		{ count(); return ')'; }
","		{ count(); return ','; }
"&"		{ count(); return '&'; }
"|"		{ count(); return '|'; }
"^"		{ count(); return '^'; }
"+"		{ count(); return '+'; }
"-"		{ count(); return '-'; }
"*"		{ count(); return '*'; }
"/"		{ count(); return '/'; }
"%"		{ count(); return '%'; }
"~"		{ count(); return '~'; }
"!"		{ count(); return '!'; }
"="		{ count(); return '='; }

[ \t\n\v\f]	{ count(); }
.		printf("Unrecognized char: %s\n", yytext);

%%

int yywrap(void) {
    return 1;
}

size_t column = 0;

void count(void) {
    for(char *s = yytext; *s; s++)
        if(*s == '\n')
            column = 0;
        else
            column++;
}
