%{
      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>
      #include <math.h>
      #include <ctype.h>
      #include <time.h>
      #include <unistd.h>
      #include <sys/types.h>
      #include <sys/stat.h>
      #include <fcntl.h>

      int letters = 0;
      int lines = 1;
      int id_count = 0;
      int int_count = 0;
      int float_count = 0;
      int op_count = 0;
      int key_count = 0;
      int comment_count = 0;
      int char_count = 0;
      int string_count = 0;
      int error_count = 0;
      int delim_count = 0;
      int other_count = 0;

      char* keywords[] = {"auto","break","case",
      "char","const","continue","default","do",
      "double","else","enum","extern","float","for","goto",
      "if","int","long","register","return","short","signed",
      "sizeof","static","struct","switch","typedef","union",
      "unsigned","void","volatile","while"};
      
      int is_keyword(char* str) {
          int i;
          for(i=0;i<32;i++){
              if(strcmp(str,keywords[i])==0)
                  return 1;
          }
          return 0;
      }
      void output() {
          printf("letters:%d\n",letters);
          printf("lines:%d\n",lines);
          printf("id_count:%d\n",id_count);
          printf("int_count:%d\n",int_count);
          printf("float_count:%d\n",float_count);
          printf("op_count:%d\n",op_count);
          printf("key_count:%d\n",key_count);
          printf("comment_count:%d\n",comment_count);
          printf("char_count:%d\n",char_count);
          printf("string_count:%d\n",string_count);
          printf("delim_count:%d\n",delim_count);
          printf("other_count:%d\n",other_count);
          printf("error_count:%d\n",error_count);
      }
%}

%option noyywrap
%option yylineno

letter          [a-zA-Z_]
digit           [0-9]
id              {letter}({letter}|{digit})*
hex_number      0[xX][0-9a-fA-F]+
oct_number      0[0-7]+
other_num       {digit}+[uUlL]|{digit}+ll[uU]|{digit}+LL[uU]|{digit}+[Ll][uU]|{digit}+[uU][Ll]|{digit}+[uU]ll|{digit}+[uU]LL
int_number      {digit}+|{digit}+[eE][+-]?{digit}+ 
float_number    {digit}+[.]{digit}+|{digit}+[.]{digit}+[eE][+-]?{digit}+
string          \"([^\\\"]|\\.)*\" 
char            \'([^\\\']|\\.)*\'
comment_line    \/\/[^\n]*\n
operator        [+\-*/%=!><&|~\^]|"++"|"+="|"--"|"-="|"->"|"/="|"%="|"<<"|"<<="|">>"|">>="|"&&"|"&="|"|="|"^="|"||"|"<="|">="|"=="|"!="|"?:"
delim           [(){}\[\];,?:]
space           [ \t\r]+
Unclosed_char   \'([^\\\']|\\.)*\n
Unclosed_string \"([^\\\"]|\\.)*\n

%%
[\xF0-\xF4][\x80-\xBF]{3} { 
    printf("Error: Unknown multi-byte character starting with byte 0x%02X in line %d\n", (unsigned char)yytext[0], yylineno); 
    letters += 1; 
    error_count++; 
}
[\xE0-\xEF][\x80-\xBF]{2} { 
    printf("Error: Unknown multi-byte character starting with byte 0x%02X in line %d\n", (unsigned char)yytext[0], yylineno); 
    letters += 1; 
    error_count++; 
}
[\xC2-\xDF][\x80-\xBF] { 
    printf("Error: Unknown multi-byte character starting with byte 0x%02X in line %d\n", (unsigned char)yytext[0], yylineno); 
    letters += 1; 
    error_count++; 
}
{comment_line}  {comment_count++;letters += yyleng + 1;lines++;}
"/*"            {
                    letters += 2;
                    char c, prev = 0;
                    while (1) {
                        c = input();            
                        if (c == EOF) {
                            error_count++;
                            printf("Error: comment block not closed at line %d\n", yylineno);
                            break;
                        }
                        letters += 1;
                        if (c == '\n') {
                            letters += 1;
                            lines++;
                        }
                        if (c == '/' && prev == '*') {
                            break;  // 找到结束符，退出循环
                        } 
                        prev = c;
                    }
                }
{id}            {
                  if(is_keyword(yytext)) {
                     key_count++;
                     printf("keyword:%s\n",yytext);
                  }
                  else {
                     id_count++;
                     printf("id:%s\n",yytext);
                  }    
                  letters += yyleng;
                } 
{int_number}      {int_count++;printf("int:%s\n",yytext);letters += yyleng;}
{float_number}    {float_count++;printf("float:%s\n",yytext);letters += yyleng;}
{oct_number}      {printf("int:%s\n",yytext);int_count++;letters += yyleng;}
{hex_number}      {printf("int:%s\n",yytext);int_count++;letters += yyleng;}
{other_num}       {printf("int:%s\n",yytext);int_count++;letters += yyleng;}
{space}           {letters += yyleng;other_count++;}
{string}          {string_count++;printf("string:%s\n",yytext);letters += yyleng;}
{char}            {char_count++;printf("char:%s\n",yytext);letters += yyleng;}
{delim}           {delim_count++;printf("delim:%s\n",yytext);letters += yyleng;}
{operator}        {op_count++;letters += yyleng;printf("op:%s\n",yytext);}
"."               {other_count++;letters += yyleng;printf("MAO:%s\n",yytext);}
"\\"              {other_count++;letters += yyleng;printf("other:%s\n",yytext);}
{Unclosed_char}   {error_count++;printf("Error: Unclosed char in line %d\n", lines);letters += yyleng + 1;lines++;}
{Unclosed_string} {error_count++;printf("Error: Unclosed string in line %d\n", lines);letters += yyleng + 1;lines++;}
\n                {lines++;letters += 2;}
#[^\n]*           {letters += yyleng;}
.                 {printf("Error: Unkown char:%s in line %d\n",yytext, yylineno);letters += yyleng;error_count++;}



%%

int main(int argc, char **argv) {
    if(argc > 1) {
      FILE *fp = fopen(argv[1],"r");
      if(fp == NULL) {
          printf("Error: Can't open file %s\n",argv[1]);
          return 1;
      }
      yyin = fp;
    }
    
    yylex();
    output();
    return 0;
}