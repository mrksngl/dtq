
#include <dtq-parse.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

int yyparse(struct NavExpr ** parsedExpression);

typedef struct yy_buffer_state * YY_BUFFER_STATE;
extern int yyparse();
extern YY_BUFFER_STATE yy_scan_string(const char * str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);

struct NavExpr * parseNavExpr(const char * expr)
{
	struct NavExpr * parsedExpression;

	YY_BUFFER_STATE buffer = yy_scan_string(expr);
	int ret = yyparse(&parsedExpression);
	yy_delete_buffer(buffer);

	if (ret) {
		return NULL;
	} else {
		return parsedExpression;
	}
}

void lexError(const char * yytext)
{
	printf("Lex Error: %s\n", yytext);
}

void yyerror(struct NavExpr ** parsedExpression, const char * s)
{
	printf("Parser Error: %s\n", s);
}

struct NavExpr * newNavExpr(char * name, struct AttrExpr * attr,
	struct NavExpr * subExpr)
{
	struct NavExpr * expr = malloc(sizeof *expr);
	assert(expr);
	expr->name = name;
	expr->attributes = attr;
	expr->subExpr = subExpr;
	return expr;
}

struct TestExpr * newTestExprExist(char * property)
{
	struct TestExpr * expr = malloc(sizeof *expr);
	assert(expr);
	expr->type = TEST_TYPE_EXIST;
	expr->property = property;
	return expr;
}

struct TestExpr * newTestExprString(enum TEST_OP op, char * property,
	char * string)
{
	struct TestExpr * expr = malloc(sizeof *expr);
	assert(expr);
	expr->type = TEST_TYPE_STR;
	expr->op = op;
	expr->property = property;
	expr->string = string;
	return expr;
}

struct TestExpr * newTestExprInteger(enum TEST_OP op, char * property,
	int integer)
{
	struct TestExpr * expr = malloc(sizeof *expr);
	assert(expr);
	expr->type = TEST_TYPE_INT;
	expr->op = op;
	expr->property = property;
	expr->integer = integer;
	return expr;
}

void freeTest(struct TestExpr * expr)
{
	if (!expr)
		return;

	if (expr->type & TEST_TYPE_STR)
		free(expr->string);
	free(expr->property);
	free(expr);
}

void freeAttributes(struct AttrExpr * expr)
{
	if (!expr)
		return;

	switch(expr->type) {
	case ATTR_TYPE_AND:
	case ATTR_TYPE_OR:
		freeAttributes(expr->left);
		freeAttributes(expr->right);
		free(expr);
		break;
	case ATTR_TYPE_NEG: {
		struct AttrExpr * subExpr = expr->neg;
		free(expr);
		freeAttributes(subExpr);
	}
		break;
	case ATTR_TYPE_TEST: {
		struct TestExpr * test = expr->test;
		free(expr);
		freeTest(test);
	}
		break;
	}
}

void freeNavExpr(struct NavExpr * expr)
{
	if (!expr)
		return;

	free(expr->name);
	freeAttributes(expr->attributes);
	struct NavExpr * next = expr->subExpr;
	free(expr);
	freeNavExpr(next); // allow leaf call
}

static const char * testOperators[] = {
	[TEST_OP_EQ] = "=",
	[TEST_OP_NE] = "!=",
	[TEST_OP_LT] = "<",
	[TEST_OP_GT] = ">",
	[TEST_OP_LE] = "<=",
	[TEST_OP_GE] = ">=",
	[TEST_OP_CONTAINS] = "~="
};

static void printTest(const struct TestExpr * test)
{
	const char * op = testOperators[test->op];
	switch (test->type) {
	case TEST_TYPE_EXIST:
		printf("%s", test->property);
		break;
	case TEST_TYPE_STR:
		printf("%s %s \"%s\"", test->property, op, test->string);
		break;
	case TEST_TYPE_INT:
		printf("%s %s 0x%x", test->property, op, test->integer);
		break;
	default:
		assert(false);
	}
}

static void printAttributes(const struct AttrExpr * attr)
{
	switch(attr->type) {
	case ATTR_TYPE_AND:
		printf("(");
		printAttributes(attr->left);
		printf(" & ");
		printAttributes(attr->right);
		printf(")");
		break;
	case ATTR_TYPE_OR:
		printf("(");
		printAttributes(attr->left);
		printf(" | ");
		printAttributes(attr->right);
		printf(")");
		break;
	case ATTR_TYPE_NEG:
		printf("!(");
		printAttributes(attr->neg);
		printf(")");
		break;
	case ATTR_TYPE_TEST:
		printTest(attr->test);
		break;
	}
}

void printNavExpr(const struct NavExpr * expr)
{
	printf("/");
	if (expr->name)
		printf("%s", expr->name);
	if (expr->attributes) {
		printf("[");
		printAttributes(expr->attributes);
		printf("]");
	}
	if (expr->subExpr)
		printNavExpr(expr->subExpr);
}
