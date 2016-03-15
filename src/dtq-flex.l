%{
#include "dtq-bison.h"
%}

%option noyywrap
%%

[][/&|!()'=()]    { return *yytext; }

\<=               { return LE; }
>=                { return GE; }
!=                { return NE; }
~=                { return CONTAINS; }

0[0-7]+           { yylval.number = strtoull(yytext, NULL, 8); return NUMBER; }

[0-9]*            { yylval.number = strtoull(yytext, NULL, 10); return NUMBER; }

0x[0-9a-fA-F]+    { yylval.number = strtoull(yytext, NULL, 16); return NUMBER; }

[a-zA-Z@_\-0-9,#]+ { yylval.text = strdup(yytext); return IDENT; }

\"[^\"]*\"        { yytext[strlen(yytext)-1] = '\0';
                    yylval.text = strdup(yytext + 1); return STRING; }

[ \t]             ;

.                 yyerror("Unknown character");

%%