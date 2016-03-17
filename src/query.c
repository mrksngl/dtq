
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <parser.h>
#include <stdbool.h>
#include <libfdt.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <assert.h>

static void printPath(const void * fdt, int offset);

static bool containsString(const char * data, int len, const char * str)
{
	const char * end = data + len;

	do {
		if (!strcmp(data, str))
			return true;
		data += strlen(data) + 1;
	} while (data < end);
	return false;
}

static bool containsInt(const char * data, int len, uint32_t i)
{
	const char * end = data + len;

	while (data + 3 < end) {
		if (fdt32_to_cpu(*(fdt32_t*)data) == i)
			return true;
		data += 4;
	}
	return false;
}

static bool testTest(const void * fdt, int offset, const struct TestExpr * test)
{
	int len;
	const struct fdt_property * prop = fdt_get_property(fdt, offset,
		test->property, &len);
	if (!prop)
		return false;

	switch (test->type) {
	case TEST_TYPE_EXIST:
		return true;
	case TEST_TYPE_STR:
		if (test->op == TEST_OP_CONTAINS) {
			return containsString(prop->data, len, test->string);
		} else {
			bool res = len == strlen(test->string) + 1;
			res = res && !memcmp(test->string, prop->data, len);
			return !(res ^ (test->op == TEST_OP_EQ));
		}
		break;
	case TEST_TYPE_INT:
		if (test->op == TEST_OP_CONTAINS) {
			return containsInt(prop->data, len, test->integer);
		} else {
			if (len != 4)
				return false;
			uint32_t i = fdt32_to_cpu(*(fdt32_t*)prop->data);

			switch (test->op) {
			case TEST_OP_EQ: return i == test->integer;
			case TEST_OP_NE: return i != test->integer;
			case TEST_OP_LE: return i < test->integer;
			case TEST_OP_GE: return i > test->integer;
			case TEST_OP_LT: return i <= test->integer;
			case TEST_OP_GT: return i >= test->integer;
			default: assert(false);
			}
		}
		break;
	default:
		assert(false);
	}
	return false;
}

static bool testAttributes(const void * fdt, int offset,
	const struct AttrExpr * attr)
{
	switch(attr->type) {
	case ATTR_TYPE_AND:
		return testAttributes(fdt, offset, attr->left) &&
			testAttributes(fdt, offset, attr->right);
	case ATTR_TYPE_OR:
		return testAttributes(fdt, offset, attr->left) ||
			testAttributes(fdt, offset, attr->right);
	case ATTR_TYPE_NEG:
		return !testAttributes(fdt, offset, attr->neg);
	case ATTR_TYPE_TEST:
		return testTest(fdt, offset, attr->test);
	default:
		return false;
	}
}

#ifndef HAVE_FDT_FIRST_SUBNODE
int fdt_first_subnode(const void *fdt, int offset)
{
	int depth = 0;

	offset = fdt_next_node(fdt, offset, &depth);
	if (offset < 0 || depth != 1)
		return -FDT_ERR_NOTFOUND;

	return offset;
}
#endif

#ifndef HAVE_FDT_NEXT_SUBNODE
int fdt_next_subnode(const void *fdt, int offset)
{
	int depth = 1;

	/*
	 * With respect to the parent, the depth of the next subnode will be
	 * the same as the last.
	 */
	do {
		offset = fdt_next_node(fdt, offset, &depth);
		if (offset < 0 || depth < 1)
			return -FDT_ERR_NOTFOUND;
	} while (depth > 1);

	return offset;
}
#endif

static void evaluate(const void * fdt, int offset, int depth,
	const struct NavExpr * expr);

static void evaluateNode(const void * fdt, int offset, int depth,
	const struct NavExpr * expr)
{
	if (expr->attributes && !testAttributes(fdt, offset, expr->attributes))
		return;
	if (expr->subExpr)
		evaluate(fdt, offset, depth, expr->subExpr);
	else
		printPath(fdt, offset);
}

static void evaluate(const void * fdt, int offset, int depth,
	const struct NavExpr * expr)
{
	switch (expr->type) {
	case NAV_EXPR_TYPE_ROOT:
		if (offset == 0)
			evaluateNode(fdt, offset, depth + 1, expr);
		break;
	case NAV_EXPR_TYPE_NODE:
		offset = fdt_first_subnode(fdt, offset);
		while (offset != -FDT_ERR_NOTFOUND) {
			if (!expr->name ||
				!strcmp(expr->name, fdt_get_name(fdt, offset, NULL)))
				evaluateNode(fdt, offset, depth + 1, expr);
			offset = fdt_next_subnode(fdt, offset);
		}
		break;
	case NAV_EXPR_TYPE_DESCEND: {
		expr = expr->subExpr;
		int cdepth = 0;
		while (true) {
			offset = fdt_next_node(fdt, offset, &cdepth);
			if (offset < 0 || cdepth < 1)
				return;
			evaluateNode(fdt, offset, depth + cdepth, expr);
		}
	}
		break;
	}
}

void queryFdt(const void * fdt, const struct NavExpr * expr)
{
	evaluate(fdt, 0, 0, expr);
}

static void printPath(const void * fdt, int offset)
{
	char path[PATH_MAX];
	fdt_get_path(fdt, offset, path, sizeof path);
	printf("Node: %s @ %d: %s\n", fdt_get_name(fdt, offset, NULL), offset, path);
}

