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
	TEST_TYPE_EXIST = 0,

	TEST_TYPE_EQ = 1,
	TEST_TYPE_NE = 2,
	TEST_TYPE_LE = 3,
	TEST_TYPE_GE = 4,
	TEST_TYPE_LT = 5,
	TEST_TYPE_GT = 6,

	TEST_TYPE_MASK = 7,

	TEST_TYPE_INT = 0,
	TEST_TYPE_STR = 8,

	TEST_TYPE_STR_EQ = TEST_TYPE_STR | TEST_TYPE_EQ,
	TEST_TYPE_STR_NE = TEST_TYPE_STR | TEST_TYPE_NE,
	TEST_TYPE_INT_EQ = TEST_TYPE_INT | TEST_TYPE_EQ,
	TEST_TYPE_INT_NE = TEST_TYPE_INT | TEST_TYPE_NE,
	TEST_TYPE_INT_LE = TEST_TYPE_INT | TEST_TYPE_LE,
	TEST_TYPE_INT_GE = TEST_TYPE_INT | TEST_TYPE_GE,
	TEST_TYPE_INT_LT = TEST_TYPE_INT | TEST_TYPE_LT,
	TEST_TYPE_INT_GT = TEST_TYPE_INT | TEST_TYPE_GT,
};

struct TestExpr {
	enum TEST_TYPE type;
	char * property;
	union {
		char * string;
		uint64_t integer;
	};
};

struct TestExpr * newTestExprString(enum TEST_TYPE type, char * property,
	char * string);

struct TestExpr * newTestExprInteger(enum TEST_TYPE type, char * property,
	int integer);

struct NavExpr * parseNavExpr(const char * expr);

void freeNavExpr(struct NavExpr * expr);

void printNavExpr(const struct NavExpr * expr);

#endif
