
#ifdef _HAVE_CONFIG_H
#include <config.h>
#endif

#include "parser.h"

#include <dtq-bison.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct yy_buffer_state * YY_BUFFER_STATE;
extern int yyparse();
extern YY_BUFFER_STATE yy_scan_string(const char * str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);

struct NodeTest * parseNodeTestExpr(const char * expr)
{
	struct NodeTest * parsedExpression;

	YY_BUFFER_STATE buffer = yy_scan_string(expr);
	int ret = yyparse(&parsedExpression, expr);
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

void yyerror(struct NodeTest ** parsedExpression, const char * expr, const char * s)
{
	const char * err = expr;
	size_t errlen = yylloc.last_column - yylloc.first_column + 1;

	size_t offset = yylloc.first_column - 1;
	size_t printLen = strlen(expr);
	bool truncate_l = false;
	bool truncate_r = false;
	if (printLen > 60) {
		if (offset >= 10) {
			err += offset - 5;
			printLen -= offset - 5;
			offset = 5;
			truncate_l = true;
		}
		if (printLen - errlen >= 10) {
			printLen = errlen + 5;
			truncate_r = true;
		}
	}

	offset += printf("%s at ", s);
	if (truncate_l) {
		offset += 1;
		printf("…");
	}
	printf("%.*s", (int)printLen, err);
	if (truncate_r)
		printf("…");
	printf("\n");

	const char spc[10] = { [0 ... 9] = ' ' };
	for (; offset >= 10; offset -= 10)
		fwrite(spc, sizeof *spc, sizeof spc, stdout);
	if (offset)
		fwrite(spc, sizeof *spc, offset, stdout);

	const char mark[10] = { [0 ... 9] = '^' };
	for (; errlen >= 10; errlen -= 10)
		fwrite(mark, sizeof *mark, sizeof mark, stdout);
	if (errlen)
		fwrite(mark, sizeof *mark, errlen, stdout);
	puts("");
}

struct NodeTest * newNodeTest(enum NODE_TEST_TYPE type, char * name,
	struct PropertyTest * attr, struct NodeTest * subExpr)
{
	struct NodeTest * expr = malloc(sizeof *expr);
	assert(expr);
	expr->type = type;
	expr->name = name;
	expr->properties = attr;
	expr->subExpr = subExpr;
	return expr;
}

struct AtomicPropertyTest * newAtomicPropertyTestExist(char * property)
{
	struct AtomicPropertyTest * expr = malloc(sizeof *expr);
	assert(expr);
	expr->type = ATOMIC_PROPERTY_TEST_TYPE_EXIST;
	expr->property = property;
	return expr;
}

struct AtomicPropertyTest * newAtomicPropertyTestString(
	enum ATOMIC_PROPERTY_TEST_OP op, char * property, char * string)
{
	struct AtomicPropertyTest * expr = malloc(sizeof *expr);
	assert(expr);
	expr->type = ATOMIC_PROPERTY_TEST_TYPE_STR;
	expr->op = op;
	expr->property = property;
	expr->string = string;
	return expr;
}

struct AtomicPropertyTest * newAtomicPropertyTestInteger(
	enum ATOMIC_PROPERTY_TEST_OP op, char * property, int integer)
{
	struct AtomicPropertyTest * expr = malloc(sizeof *expr);
	assert(expr);
	expr->type = ATOMIC_PROPERTY_TEST_TYPE_INT;
	expr->op = op;
	expr->property = property;
	expr->integer = integer;
	return expr;
}

void freeAtomicPropertyTest(struct AtomicPropertyTest * test)
{
	if (!test)
		return;

	if (test->type == ATOMIC_PROPERTY_TEST_TYPE_STR)
		free(test->string);
	free(test->property);
	free(test);
}

void freePropertyTest(struct PropertyTest * test)
{
	if (!test)
		return;

	switch (test->type) {
	case PROPERTY_TEST_OP_AND:
	case PROPERTY_TEST_OP_OR:
		freePropertyTest(test->left);
		freePropertyTest(test->right);
		free(test);
		break;
	case PROPERTY_TEST_OP_NEG: {
		struct PropertyTest * subTest = test->neg;
		free(test);
		freePropertyTest(subTest);
	}
		break;
	case PROPERTY_TEST_OP_TEST: {
		struct AtomicPropertyTest * subTest = test->test;
		free(subTest);
		freeAtomicPropertyTest(subTest);
	}
		break;
	}
}

void freeNodeTest(struct NodeTest * expr)
{
	if (!expr)
		return;

	free(expr->name);
	freePropertyTest(expr->properties);
	struct NodeTest * next = expr->subExpr;
	free(expr);
	freeNodeTest(next); // allow leaf call
}

static const char * testOperators[] = {
	[ATOMIC_PROPERTY_TEST_OP_EQ] = "=",
	[ATOMIC_PROPERTY_TEST_OP_NE] = "!=",
	[ATOMIC_PROPERTY_TEST_OP_LE] = "<=",
	[ATOMIC_PROPERTY_TEST_OP_GE] = ">=",
	[ATOMIC_PROPERTY_TEST_OP_LT] = "<",
	[ATOMIC_PROPERTY_TEST_OP_GT] = ">",
	[ATOMIC_PROPERTY_TEST_OP_CONTAINS] = "~="
};

static void printAtomicPropertyTest(const struct AtomicPropertyTest * test)
{
	const char * op = testOperators[test->op];
	switch (test->type) {
	case ATOMIC_PROPERTY_TEST_TYPE_EXIST:
		printf("%s", test->property);
		break;
	case ATOMIC_PROPERTY_TEST_TYPE_INT:
		printf("%s %s 0x%x", test->property, op, test->integer);
		break;
	case ATOMIC_PROPERTY_TEST_TYPE_STR:
		printf("%s %s \"%s\"", test->property, op, test->string);
		break;
	default:
		assert(false);
	}
}

static void printPropertyTest(const struct PropertyTest * test)
{
	switch (test->type) {
	case PROPERTY_TEST_OP_AND:
		printf("(");
		printPropertyTest(test->left);
		printf(" & ");
		printPropertyTest(test->right);
		printf(")");
		break;
	case PROPERTY_TEST_OP_OR:
		printf("(");
		printPropertyTest(test->left);
		printf(" | ");
		printPropertyTest(test->right);
		printf(")");
		break;
	case PROPERTY_TEST_OP_NEG:
		printf("!(");
		printPropertyTest(test->neg);
		printf(")");
		break;
	case PROPERTY_TEST_OP_TEST:
		printAtomicPropertyTest(test->test);
		break;
	}
}

void printNodeTest(const struct NodeTest * expr)
{
	if (expr->type != NODE_TEST_TYPE_ROOT)
		printf("/");

	if (expr->type != NODE_TEST_TYPE_DESCEND && expr->name)
		printf("%s", expr->name);

	if (expr->properties) {
		printf("[");
		printPropertyTest(expr->properties);
		printf("]");
	}

	if (expr->subExpr)
		printNodeTest(expr->subExpr);
}
