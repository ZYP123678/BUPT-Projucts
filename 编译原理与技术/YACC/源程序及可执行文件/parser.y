%{
#include <stdio.h>
#include <stdlib.h>
#define YYLEX yylex
void yyerror(const char *s);
int yylex();
%}

%token NUM
%left '+' '-'
%left '*' '/'
%right UMINUS

%%  
line:
    expr '\n' { printf("Result: %d\nExpression is valid.\n", $1); }
;

expr:
    NUM { $$ = $1; }
    | expr '+' expr { $$ = $1 + $3; }
    | expr '-' expr { $$ = $1 - $3; }
    | expr '*' expr { $$ = $1 * $3; }
    | expr '/' expr { $$ = $1 / $3; }
    | '(' expr ')' { $$ = $2; }
;

%% 

int main() {
    printf("Enter an expression: ");
    if (yyparse() == 0) {
    } else {
        printf("Expression is invalid.\n");
    }
    return 0;
}

void yyerror(const char *s) {
    fprintf(stderr, "Error: %s\n", s);
}
