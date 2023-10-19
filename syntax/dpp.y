 /* vim: noet sw=8 ts=8
  */
%token IDENTIFIER CONSTANT
%token LEQ_OP GEQ_OP EQ_OP NEQ_OP LAND_OP LOR_OP

%token LET IF ELSE FN RETURN

%start translation_unit

%{
#include "lex.yy.c"

#ifdef DPP_DEBUG_
# undef YYDEBUG
# define YYDEBUG 1
int yydebug = 1;
#endif

int yyerror(char *s);
%}

%%

primary_expr
	: IDENTIFIER
	| CONSTANT
	| '(' expr ')'
	;

argument_expr_list
	: expr
	| argument_expr_list ',' expr
	;

function_call
	: IDENTIFIER '(' ')'
	| IDENTIFIER '(' argument_expr_list ')'
	;

unary_expr
	: primary_expr
	| function_call
	| '!' function_call
	| '~' function_call
	;

mult_expr
	: unary_expr
	| mult_expr '*' unary_expr
	| mult_expr '/' unary_expr
	| mult_expr '%' unary_expr
	;

add_expr
	: mult_expr
	| add_expr '+' mult_expr
	| add_expr '-' mult_expr
	;

rel_expr
	: add_expr
	| rel_expr '<' add_expr
	| rel_expr '>' add_expr
	| rel_expr LEQ_OP add_expr
	| rel_expr GEQ_OP add_expr
	;

eq_expr
	: rel_expr
	| eq_expr EQ_OP rel_expr
	| eq_expr NEQ_OP rel_expr
	;

bitand_expr
	: eq_expr
	| bitand_expr '&' eq_expr
	;

bitxor_expr
	: bitand_expr
	| bitxor_expr '^' bitand_expr
	;

bitor_expr
	: bitxor_expr
	| bitor_expr '|' bitxor_expr
	;

land_expr
	: bitor_expr
	| land_expr LAND_OP bitor_expr
	;

lor_expr
	: land_expr
	| lor_expr LOR_OP land_expr
	;

expr
	: lor_expr
	;

expr_statement
	: ';'
	| expr ';'
	;

if_statement
	: IF expr block_statement
	| IF expr block_statement ELSE block_statement
	| IF expr block_statement ELSE if_statement
	;

ret_statement
	: RETURN expr ';'
	;

declaration
	: LET IDENTIFIER '=' expr ';'
	;

statement
	: declaration
	| expr_statement
	| if_statement
	| ret_statement
	| block_statement
	;

statement_list
	: statement
	| statement_list statement
	;

block_statement
	: '{' '}'
	| '{' statement_list '}'
	;

argument_list
	: IDENTIFIER
	| argument_list ',' IDENTIFIER
	;

fn_definition
	: FN IDENTIFIER '(' ')' block_statement
	| FN IDENTIFIER '(' argument_list ')' block_statement
	;

translation_unit
	: fn_definition
	| translation_unit fn_definition
	;

%%

extern size_t column;
extern FILE *yyin;

int yyerror(char *s) {
    fflush(stdout);
    /* this does not work */
    printf("\n%*s\n%*s\n", column, "^", column, s);

    return 0;
}

int main(int argc, char *argv[]) {
    if(argc > 1)
        yyin = fopen(argv[1], "r");
    else
        yyin = stdin;

    return yyparse();
}
