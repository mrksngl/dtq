%{
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <parser.h>

extern int yylex();
%}

%parse-param {struct NodeTest ** parsedNodeTest} {const char * unparsedExpression}
%locations

%union {
  uint64_t number;
  char * text;
  struct NodeTest * node;
  struct PropertyTest * properties;
  struct AtomicPropertyTest * atom;
};

%type <node> path
%type <properties> properties propertyExpr
%type <atom> property

%token <number> NUMBER
%token <text> IDENT STRING
%token LE GE NE CONTAINS

%destructor { freeNodeTest($$); } <node>
%destructor { freePropertyTest($$); } <properties>
%destructor { freeAtomicPropertyTest($$); } <atom>


%left '|'
%left '&'
%left '!'

%%

start: path { *parsedNodeTest = newNodeTest(NODE_TEST_TYPE_ROOT, NULL, NULL, $1); };

path:
  '/' path                  { $$ = newNodeTest(NODE_TEST_TYPE_DESCEND, NULL, NULL, $2); }
 |'/' IDENT properties path { $$ = newNodeTest(NODE_TEST_TYPE_NODE, $2, $3, $4); }
 |'/' IDENT path            { $$ = newNodeTest(NODE_TEST_TYPE_NODE, $2, NULL, $3); }
 |'/' properties path       { $$ = newNodeTest(NODE_TEST_TYPE_NODE, NULL, $2, $3); }
 |                          { $$ = NULL; }
 ;

properties: '[' propertyExpr ']' { $$ = $2; };

propertyExpr:
  '(' propertyExpr ')' { $$ = $2; }
 |propertyExpr '&' propertyExpr
    { struct PropertyTest * test = malloc(sizeof *test);
      test->type = PROPERTY_TEST_OP_AND;
      test->left = $1;
      test->right = $3;
      $$ = test;
    }
 |propertyExpr '|' propertyExpr
    { struct PropertyTest * test = malloc(sizeof *test);
      test->type = PROPERTY_TEST_OP_OR;
      test->left = $1;
      test->right = $3;
      $$ = test;
    }
 |'!'propertyExpr
    { struct PropertyTest * test = malloc(sizeof *test);
      test->type = PROPERTY_TEST_OP_NEG;
      test->neg = $2;
      $$ = test;
    }
 |property
    { struct PropertyTest * test = malloc(sizeof *test);
      test->type = PROPERTY_TEST_OP_TEST;
      test->test = $1;
      $$ = test;
    }
 |  { $$ = NULL; }
 ;

property:
  IDENT { $$ = newAtomicPropertyTestExist($1); }
 |IDENT '=' NUMBER { $$ = newAtomicPropertyTestInteger(ATOMIC_PROPERTY_TEST_OP_EQ, $1, $3); }
 |IDENT NE NUMBER { $$ = newAtomicPropertyTestInteger(ATOMIC_PROPERTY_TEST_OP_NE, $1, $3); }
 |IDENT LE NUMBER { $$ = newAtomicPropertyTestInteger(ATOMIC_PROPERTY_TEST_OP_LE, $1, $3); }
 |IDENT GE NUMBER { $$ = newAtomicPropertyTestInteger(ATOMIC_PROPERTY_TEST_OP_GE, $1, $3); }
 |IDENT '<' NUMBER { $$ = newAtomicPropertyTestInteger(ATOMIC_PROPERTY_TEST_OP_LT, $1, $3); }
 |IDENT '>' NUMBER { $$ = newAtomicPropertyTestInteger(ATOMIC_PROPERTY_TEST_OP_GT, $1, $3); }
 |IDENT CONTAINS NUMBER { $$ = newAtomicPropertyTestInteger(ATOMIC_PROPERTY_TEST_OP_CONTAINS, $1, $3); }
 |IDENT '=' STRING { $$ = newAtomicPropertyTestString(ATOMIC_PROPERTY_TEST_OP_EQ, $1, $3); }
 |IDENT NE STRING { $$ = newAtomicPropertyTestString(ATOMIC_PROPERTY_TEST_OP_NE, $1, $3); }
 |IDENT CONTAINS STRING { $$ = newAtomicPropertyTestString(ATOMIC_PROPERTY_TEST_OP_CONTAINS, $1, $3); }
 ;
