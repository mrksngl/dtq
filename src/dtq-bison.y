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

/* terminal and non-terminal types */
%union {
  uint64_t number;
  char * text;
  struct NodeTest * node;
  struct PropertyTest * properties;
  struct AtomicPropertyTest * atom;
};

/* non-terminal types */
%type <node> node
%type <properties> properties propertyExpr
%type <atom> property

/* terminals and their types */
%token <number> NUMBER
%token <text> IDENT STRING
%token LE GE NE CONTAINS

/* destructors, in case of failure */
%destructor { freeNodeTest($$); } <node>
%destructor { freePropertyTest($$); } <properties>
%destructor { freeAtomicPropertyTest($$); } <atom>

/* operator precedences and associativity */
%left '|'
%left '&'
%left '!'

%%

/* start-symbol is a node test */
start:
  node
    { *parsedNodeTest = newNodeTest(NODE_TEST_TYPE_ROOT, NULL, NULL, $1); };

/* a node is ... */
node:
  '/' node                  /* an empty test -> descend */
    { $$ = newNodeTest(NODE_TEST_TYPE_DESCEND, NULL, NULL, $2); } 
 |'/' IDENT properties node /* a node test with a name and properties */ 
    { $$ = newNodeTest(NODE_TEST_TYPE_NODE, $2, $3, $4); }
 |'/' IDENT node            /* a node test with a name without properties */
    { $$ = newNodeTest(NODE_TEST_TYPE_NODE, $2, NULL, $3); }
 |'/' properties node       /* a node test without a name but with properties */
    { $$ = newNodeTest(NODE_TEST_TYPE_NODE, NULL, $2, $3); }
 |                          /* empty -> done */
    { $$ = NULL; }
 ;

/* properties are enclosed by brackets */ 
properties: '[' propertyExpr ']' { $$ = $2; };

/* a compound property is ... */
propertyExpr:
  '(' propertyExpr ')' /* another property in brackets */ { $$ = $2; }
 |propertyExpr '&' propertyExpr /* two properties in a conjunction */
    { $$ = newPropertyTestBinary(PROPERTY_TEST_OP_AND, $1, $3); }
 |propertyExpr '|' propertyExpr /* two properties in a disjunction */
    { $$ = newPropertyTestBinary(PROPERTY_TEST_OP_OR, $1, $3); }
 |'!'propertyExpr /* a negated property */
    { $$ = newPropertyTestUnary(PROPERTY_TEST_OP_NEG, $2); }
 |property /* an atomic property */
    { $$ = newPropertyTestAtomic($1); }
 | /* empty */ { $$ = NULL; }
 ;

/* an atomic property is ... */
property:
  IDENT /* a test for existence */ { $$ = newAtomicPropertyTestExist($1); }
 |IDENT '=' NUMBER      /* an "equality"-test of integers */
    { $$ = newAtomicPropertyTestInteger(ATOMIC_PROPERTY_TEST_OP_EQ, $1, $3); }
 |IDENT NE NUMBER       /* an "inequality"-test of integers */
    { $$ = newAtomicPropertyTestInteger(ATOMIC_PROPERTY_TEST_OP_NE, $1, $3); }
 |IDENT LE NUMBER       /* a "less or equal"-test of integers */
    { $$ = newAtomicPropertyTestInteger(ATOMIC_PROPERTY_TEST_OP_LE, $1, $3); }
 |IDENT GE NUMBER       /* a "greater or equal"-test of integers */ 
    { $$ = newAtomicPropertyTestInteger(ATOMIC_PROPERTY_TEST_OP_GE, $1, $3); }
 |IDENT '<' NUMBER      /* a "less than"-test of integers */
    { $$ = newAtomicPropertyTestInteger(ATOMIC_PROPERTY_TEST_OP_LT, $1, $3); }
 |IDENT '>' NUMBER      /* a "greater than"-test of integers */
    { $$ = newAtomicPropertyTestInteger(ATOMIC_PROPERTY_TEST_OP_GT, $1, $3); }
 |IDENT CONTAINS NUMBER /* a "contains"-test on an integer array */
    { $$ = newAtomicPropertyTestInteger(ATOMIC_PROPERTY_TEST_OP_CONTAINS, $1,
      $3); }
 |IDENT '=' STRING      /* an "equality"-test of strings */
    { $$ = newAtomicPropertyTestString(ATOMIC_PROPERTY_TEST_OP_EQ, $1, $3); }
 |IDENT NE STRING       /* an "inequality"-test of strings */
    { $$ = newAtomicPropertyTestString(ATOMIC_PROPERTY_TEST_OP_NE, $1, $3); }
 |IDENT CONTAINS STRING /* a "contains"-test on a string array */
    { $$ = newAtomicPropertyTestString(ATOMIC_PROPERTY_TEST_OP_CONTAINS, $1,
      $3); }
 ;
