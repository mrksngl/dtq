
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
static void query(const void * fdt, int offset, int depth,
	const struct NodeTest * test);

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

static bool queryAtomicPropertyTest(const void * fdt, int offset,
	const struct AtomicPropertyTest * test)
{
	int len;
	const struct fdt_property * prop = fdt_get_property(fdt, offset,
		test->property, &len);
	if (!prop)
		return false;

	switch (test->type) {
	case ATOMIC_PROPERTY_TEST_TYPE_EXIST:
		return true;
	case ATOMIC_PROPERTY_TEST_TYPE_INT:
		if (test->op == ATOMIC_PROPERTY_TEST_OP_CONTAINS) {
			return containsInt(prop->data, len, test->integer);
		} else {
			if (len != 4)
				return false;
			uint32_t i = fdt32_to_cpu(*(fdt32_t*)prop->data);

			switch (test->op) {
			case ATOMIC_PROPERTY_TEST_OP_EQ: return i == test->integer;
			case ATOMIC_PROPERTY_TEST_OP_NE: return i != test->integer;
			case ATOMIC_PROPERTY_TEST_OP_LE: return i <= test->integer;
			case ATOMIC_PROPERTY_TEST_OP_GE: return i >= test->integer;
			case ATOMIC_PROPERTY_TEST_OP_LT: return i < test->integer;
			case ATOMIC_PROPERTY_TEST_OP_GT: return i > test->integer;
			default: assert(false);
			}
		}
		break;
	case ATOMIC_PROPERTY_TEST_TYPE_STR:
		if (test->op == ATOMIC_PROPERTY_TEST_OP_CONTAINS) {
			return containsString(prop->data, len, test->string);
		} else {
			bool res = len == strlen(test->string) + 1;
			res = res && !memcmp(test->string, prop->data, len);
			return !(res ^ (test->op == ATOMIC_PROPERTY_TEST_OP_EQ));
		}
		break;
	default:
		assert(false);
	}
	return false;
}

static bool queryPropertyTest(const void * fdt, int offset,
	const struct PropertyTest * attr)
{
	switch (attr->type) {
	case PROPERTY_TEST_OP_AND:
		return queryPropertyTest(fdt, offset, attr->left) &&
			queryPropertyTest(fdt, offset, attr->right);
	case PROPERTY_TEST_OP_OR:
		return queryPropertyTest(fdt, offset, attr->left) ||
			queryPropertyTest(fdt, offset, attr->right);
	case PROPERTY_TEST_OP_NEG:
		return !queryPropertyTest(fdt, offset, attr->child);
	case PROPERTY_TEST_OP_ATOMIC:
		return queryAtomicPropertyTest(fdt, offset, attr->atomic);
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

static void queryNode(const void * fdt, int offset, int depth,
	const struct NodeTest * test)
{
	if (test->properties && !queryPropertyTest(fdt, offset, test->properties))
		return;
	if (test->subExpr)
		query(fdt, offset, depth, test->subExpr);
	else
		printPath(fdt, offset);
}

static void query(const void * fdt, int offset, int depth,
	const struct NodeTest * test)
{
	switch (test->type) {
	case NODE_TEST_TYPE_ROOT:
		if (offset == 0)
			queryNode(fdt, offset, depth + 1, test);
		break;
	case NODE_TEST_TYPE_NODE:
		offset = fdt_first_subnode(fdt, offset);
		while (offset != -FDT_ERR_NOTFOUND) {
			if (!test->name ||
				!strcmp(test->name, fdt_get_name(fdt, offset, NULL)))
				queryNode(fdt, offset, depth + 1, test);
			offset = fdt_next_subnode(fdt, offset);
		}
		break;
	case NODE_TEST_TYPE_DESCEND: {
		test = test->subExpr;
		int cdepth = 0;
		while (true) {
			offset = fdt_next_node(fdt, offset, &cdepth);
			if (offset < 0 || cdepth < 1)
				return;
			queryNode(fdt, offset, depth + cdepth, test);
		}
	}
		break;
	}
}

void queryFdt(const void * fdt, const struct NodeTest * expr)
{
	query(fdt, 0, 0, expr);
}

static void printPath(const void * fdt, int offset)
{
	char path[PATH_MAX];
	fdt_get_path(fdt, offset, path, sizeof path);
	printf("Node: %s @ %d: %s\n", fdt_get_name(fdt, offset, NULL), offset, path);
}

