%{
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dtq-bison.h"
#include <parser.h>

#define YY_NO_INPUT
#define YY_NO_UNPUT

static int yycolumn = 1;

#define YY_USER_ACTION yylloc.first_column = yycolumn; \
	yylloc.last_column = yycolumn + yyleng - 1; \
	yycolumn += yyleng; \

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

.                 lexError(yytext);

%%