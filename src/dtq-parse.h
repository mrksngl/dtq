#ifndef _DTQ_H
#define _DTQ_H

#include <stdint.h>

struct AttrExpr;
struct TestExpr;

struct NavExpr {
	char * name;
	struct AttrExpr * attributes;
	struct NavExpr * subExpr;
};

enum ATTR_TYPE {
	ATTR_TYPE_AND,
	ATTR_TYPE_OR,
	ATTR_TYPE_NEG,
	ATTR_TYPE_TEST,
};

struct AttrExpr {
	enum ATTR_TYPE type;
	union {
		struct {
			struct AttrExpr * left;
			struct AttrExpr * right;
		};
		struct AttrExpr * neg;
		struct TestExpr * test;
	};
};

enum TEST_TYPE {
	TEST_TYPE_EXIST,
	TEST_TYPE_INT,
	TEST_TYPE_STR
};

enum TEST_OP {
	TEST_OP_EQ,
	TEST_OP_NE,
	TEST_OP_LE,
	TEST_OP_GE = 4,
	TEST_OP_LT = 5,
	TEST_OP_GT = 6,
	TEST_OP_CONTAINS = 7,
};

struct TestExpr {
	enum TEST_TYPE type;
	enum TEST_OP op;
	char * property;
	union {
		char * string;
		uint32_t integer;
	};
};

struct TestExpr * newTestExprExist(char * property);

struct TestExpr * newTestExprString(enum TEST_OP op, char * property,
	char * string);

struct TestExpr * newTestExprInteger(enum TEST_OP op, char * property,
	int integer);

struct NavExpr * parseNavExpr(const char * expr);

void freeNavExpr(struct NavExpr * expr);

void freeTest(struct TestExpr * expr);

void freeAttributes(struct AttrExpr * expr);

void printNavExpr(const struct NavExpr * expr);

#endif
