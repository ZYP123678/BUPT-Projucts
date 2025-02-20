%{
#include <stdio.h>
#include <stdlib.h>
#include "y.tab.h"
%}

%%

[0-9]+(\.[0-9]+)?  { yylval = atoi(yytext); return NUM; }
[ \t]              ;  // 忽略空格
\n                 { return '\n'; }
"+"                {return '+';}
"-"                {return '-';}
"*"                {return '*';}
"/"                {return '/';}
"("                {return '(';}
")"                {return ')';}
.                  { return yytext[0]; }
%%

int yywrap() {
    return 1;
}
