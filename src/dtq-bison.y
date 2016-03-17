%{
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <parser.h>

extern int yylex();
%}

%parse-param {struct NavExpr ** parsedExpression} {const char * unparsedExpression}
%locations

%union {
  uint64_t number;
  char * text;
  struct NavExpr * navExpr;
  struct AttrExpr * attrExpr;
  struct TestExpr * testExpr;
};

%type <navExpr> path
%type <attrExpr> attributes
%type <attrExpr> attributeExpr
%type <testExpr> test

%token <number> NUMBER
%token <text> IDENT STRING
%token LE GE NE CONTAINS

%destructor { freeNavExpr($$); } <navExpr>
%destructor { freeAttributes($$); } <attrExpr>
%destructor { freeTest($$); } <testExpr>


%left '|'
%left '&'
%left '!'

%%

start: path { *parsedExpression = newNavExpr(NAV_EXPR_TYPE_ROOT, NULL, NULL, $1); };

path:
  '/' path                  { $$ = newNavExpr(NAV_EXPR_TYPE_DESCEND, NULL, NULL, $2); }
 |'/' IDENT attributes path { $$ = newNavExpr(NAV_EXPR_TYPE_NODE, $2, $3, $4); }
 |'/' IDENT path            { $$ = newNavExpr(NAV_EXPR_TYPE_NODE, $2, NULL, $3); }
 |'/' attributes path       { $$ = newNavExpr(NAV_EXPR_TYPE_NODE, NULL, $2, $3); }
 |                          { $$ = NULL; }
 ;

attributes: '[' attributeExpr ']' { $$ = $2; };

attributeExpr:
  '(' attributeExpr ')' { $$ = $2; }
 |attributeExpr '&' attributeExpr
    { struct AttrExpr * expr = malloc(sizeof *expr);
      expr->type = ATTR_TYPE_AND;
      expr->left = $1;
      expr->right = $3;
      $$ = expr;
    }
 |attributeExpr '|' attributeExpr
    { struct AttrExpr * expr = malloc(sizeof *expr);
      expr->type = ATTR_TYPE_OR;
      expr->left = $1;
      expr->right = $3;
      $$ = expr;
    }
 |'!'attributeExpr
    { struct AttrExpr * expr = malloc(sizeof *expr);
      expr->type = ATTR_TYPE_NEG;
      expr->neg = $2;
      $$ = expr;
    }
 |test
    { struct AttrExpr * expr = malloc(sizeof *expr);
      expr->type = ATTR_TYPE_TEST;
      expr->test = $1;
      $$ = expr;
    }
 |  { $$ = NULL; }
 ;

test:
  IDENT { $$ = newTestExprExist($1); }
 |IDENT '=' STRING { $$ = newTestExprString(TEST_OP_EQ, $1, $3); }
 |IDENT NE STRING { $$ = newTestExprString(TEST_OP_NE, $1, $3); }
 |IDENT CONTAINS STRING { $$ = newTestExprString(TEST_OP_CONTAINS, $1, $3); }
 |IDENT '=' NUMBER { $$ = newTestExprInteger(TEST_OP_EQ, $1, $3); }
 |IDENT CONTAINS NUMBER { $$ = newTestExprInteger(TEST_OP_CONTAINS, $1, $3); }
 |IDENT '>' NUMBER { $$ = newTestExprInteger(TEST_OP_GT, $1, $3); }
 |IDENT '<' NUMBER { $$ = newTestExprInteger(TEST_OP_LT, $1, $3); }
 |IDENT NE NUMBER { $$ = newTestExprInteger(TEST_OP_NE, $1, $3); }
 |IDENT LE NUMBER { $$ = newTestExprInteger(TEST_OP_LE, $1, $3); }
 |IDENT GE NUMBER { $$ = newTestExprInteger(TEST_OP_GE, $1, $3); }
 ;
