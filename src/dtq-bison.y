%{
#include <stdio.h>
#include <dtq-parse.h>

void yyerror(char *s) {
  fprintf(stderr, "%s\n", s);
}

extern struct NavExpr * parsedExpression;

%}

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

%left '|'
%left '&'
%left '!'

%%

start: path { parsedExpression = $1; };

path:
  '/' IDENT attributes path
    { struct NavExpr * expr = malloc(sizeof *expr);
      expr->name = $2;
      expr->attributes = $3;
      expr->subExpr = $4;
      $$ = expr;
    }
 |'/' IDENT attributes
    { struct NavExpr * expr = malloc(sizeof *expr);
      expr->name = $2;
      expr->attributes = $3;
      expr->subExpr = NULL;
      $$ = expr;
    }
 ;

attributes: '[' attributeExpr ']' { $$ = $2; } | { $$ = NULL; };

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
  IDENT { $$ = newTestExprString(TEST_TYPE_EXIST, $1, NULL); }
 |IDENT '=' STRING { $$ = newTestExprString(TEST_TYPE_STR_EQ, $1, $3); }
 |IDENT NE STRING { $$ = newTestExprString(TEST_TYPE_STR_NE, $1, $3); }
 |IDENT CONTAINS STRING { $$ = newTestExprString(TEST_TYPE_STR_CONTAINS, 
                          $1, $3); }
 |IDENT '=' NUMBER { $$ = newTestExprInteger(TEST_TYPE_INT_EQ, $1, $3); }
 |IDENT '>' NUMBER { $$ = newTestExprInteger(TEST_TYPE_INT_GT, $1, $3); }
 |IDENT '<' NUMBER { $$ = newTestExprInteger(TEST_TYPE_INT_LT, $1, $3); }
 |IDENT NE NUMBER { $$ = newTestExprInteger(TEST_TYPE_INT_NE, $1, $3); }
 |IDENT LE NUMBER { $$ = newTestExprInteger(TEST_TYPE_INT_LE, $1, $3); }
 |IDENT GE NUMBER { $$ = newTestExprInteger(TEST_TYPE_INT_GE, $1, $3); }
 ;
