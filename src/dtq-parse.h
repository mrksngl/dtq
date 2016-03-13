#ifndef _DTQ_H
#define _DTQ_H

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
	TEST_TYPE_STR_EQ,
	TEST_TYPE_STR_NE,
	TEST_TYPE_INT_EQ,
	TEST_TYPE_INT_NE,
	TEST_TYPE_INT_LE,
	TEST_TYPE_INT_GE,
	TEST_TYPE_INT_LT,
	TEST_TYPE_INT_GT,
};

struct TestExpr {
	enum TEST_TYPE type;
	char * property;
	union {
		char * string;
		int integer;
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
