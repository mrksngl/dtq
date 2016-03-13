
#include <dtq-parse.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

int yyparse(void);

typedef struct yy_buffer_state * YY_BUFFER_STATE;
extern int yyparse();
extern YY_BUFFER_STATE yy_scan_string(const char * str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);

struct NavExpr * parsedExpression;

struct NavExpr * parseNavExpr(const char * expr)
{
    YY_BUFFER_STATE buffer = yy_scan_string(expr);
    yyparse();
    yy_delete_buffer(buffer);
    return parsedExpression;
}

struct TestExpr * newTestExprString(enum TEST_TYPE type, char * property,
	char * string)
{
	struct TestExpr * expr = malloc(sizeof *expr);
	assert(expr);
	expr->type = type;
	expr->property = property;
	expr->string = string;
	return expr;
}

struct TestExpr * newTestExprInteger(enum TEST_TYPE type, char * property,
	int integer)
{
	struct TestExpr * expr = malloc(sizeof *expr);
	assert(expr);
	expr->type = type;
	expr->property = property;
	expr->integer = integer;
	return expr;
}

static void freeTest(struct TestExpr * expr)
{
	if (!expr)
		return;

	switch(expr->type) {
	case TEST_TYPE_STR_EQ:
	case TEST_TYPE_STR_NE:
		free(expr->string);
		break;
	default:
		break;
	}
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
	[TEST_TYPE_STR_EQ] = "=",
	[TEST_TYPE_STR_NE] = "!=",
	[TEST_TYPE_INT_EQ] = "=",
	[TEST_TYPE_INT_NE] = "!=",
	[TEST_TYPE_INT_LT] = "<",
	[TEST_TYPE_INT_GT] = ">",
	[TEST_TYPE_INT_LE] = "<=",
	[TEST_TYPE_INT_GE] = ">=",
};

static void printTest(const struct TestExpr * test)
{
	switch(test->type) {
	case TEST_TYPE_EXIST:
		printf("%s", test->property);
		break;
	case TEST_TYPE_STR_EQ:
	case TEST_TYPE_STR_NE:
		printf("%s %s \"%s\"", test->property, testOperators[test->type],
			test->string);
		break;
	case TEST_TYPE_INT_EQ:
	case TEST_TYPE_INT_NE:
	case TEST_TYPE_INT_LT:
	case TEST_TYPE_INT_GT:
	case TEST_TYPE_INT_LE:
	case TEST_TYPE_INT_GE:
		printf("%s %s 0x%x", test->property, testOperators[test->type],
			test->integer);
		break;
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
