#ifndef _PARSER_H
#define _PARSER_H

#include <stdint.h>

/** Data Type of an atomic property test */
enum ATOMIC_PROPERTY_TEST_TYPE {
	/** Test for existence (no data type needed) */
	ATOMIC_PROPERTY_TEST_TYPE_EXIST,
	/** Test on an integer or integer array */
	ATOMIC_PROPERTY_TEST_TYPE_INT,
	/** Test on a string or string array */
	ATOMIC_PROPERTY_TEST_TYPE_STR
};

/** Comparison operator of an atomic property test */
enum ATOMIC_PROPERTY_TEST_OP {
	/** Test on equality */
	ATOMIC_PROPERTY_TEST_OP_EQ,
	/** Test on inequality */
	ATOMIC_PROPERTY_TEST_OP_NE,
	/** Test "less or equal" */
	ATOMIC_PROPERTY_TEST_OP_LE,
	/** Test "greater or equal */
	ATOMIC_PROPERTY_TEST_OP_GE,
	/** Test "less than" */
	ATOMIC_PROPERTY_TEST_OP_LT,
	/** Test "greater than" */
	ATOMIC_PROPERTY_TEST_OP_GT,
	/** Test equality of one element in an array */
	ATOMIC_PROPERTY_TEST_OP_CONTAINS
};

/** AST: Atomic Property Test */
struct AtomicPropertyTest {
	/** Data Type */
	enum ATOMIC_PROPERTY_TEST_TYPE type;
	/** Comparison operator */
	enum ATOMIC_PROPERTY_TEST_OP op;
	/** Property name */
	char * property;
	union {
		/** Data to compare to: string */
		char * string;
		/** Data to compare to: integer */
		uint32_t integer;
	};
};

/** Property test operation */
enum PROPERTY_TEST_OP {
	/** Conjunction of two tests */
	PROPERTY_TEST_OP_AND,
	/** Disjunction of two tests */
	PROPERTY_TEST_OP_OR,
	/** Negation of a test */
	PROPERTY_TEST_OP_NEG,
	/** An atomic test (-> leaf of AST) */
	PROPERTY_TEST_OP_TEST
};

/** AST: Property Test (i.e. logical conjunction of (atomic) property tests) */
struct PropertyTest {
	/** Conjunction type */
	enum PROPERTY_TEST_OP type;
	union {
		struct {
			/** for binary operators: left child */
			struct PropertyTest * left;
			/** for binary operators: right child */
			struct PropertyTest * right;
		};
		/** for unary operators: child */
		struct PropertyTest * neg;
		/** for atomic tests: leaf */
		struct AtomicPropertyTest * test;
	};
};

/** Node Test Type */
enum NODE_TEST_TYPE {
	/** Root node */
	NODE_TEST_TYPE_ROOT,
	/** Simple node */
	NODE_TEST_TYPE_NODE,
	/** Descending node (recurses into ALL subnodes) */
	NODE_TEST_TYPE_DESCEND,
};

/** AST: Node Test */
struct NodeTest {
	/** node type */
	enum NODE_TEST_TYPE type;
	/** optional: node name to match */
	char * name;
	/** optional: node properties */
	struct PropertyTest * properties;
	/** optional: child node test. Required for type NODE_TEST_TYPE_DESCEND */
	struct NodeTest * subExpr;
};

void lexError(const char * yytext);

void yyerror(struct NodeTest ** parsedExpression, const char * expr,
	const char * s);

struct NodeTest * newNodeTest(enum NODE_TEST_TYPE type, char * name,
	struct PropertyTest * properties, struct NodeTest * subExpr);

struct AtomicPropertyTest * newAtomicPropertyTestExist(char * property);

struct AtomicPropertyTest * newAtomicPropertyTestString(
	enum ATOMIC_PROPERTY_TEST_OP op, char * property, char * string);

struct AtomicPropertyTest * newAtomicPropertyTestInteger(
	enum ATOMIC_PROPERTY_TEST_OP op, char * property, int integer);

struct NodeTest * parseNodeTestExpr(const char * expr);

void freeNodeTest(struct NodeTest * test);

void freeAtomicPropertyTest(struct AtomicPropertyTest * test);

void freePropertyTest(struct PropertyTest * test);

void printNodeTest(const struct NodeTest * test);

#endif
