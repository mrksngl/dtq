
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

/** Test a node which is already a candidate for the overall test or for
 * further recursion.
 * \param fdt flattened device tree
 * \param offset offset to the current node
 * \param depth depth of the current node
 * \param test current node test
 */
static void queryNode(const void * fdt, int offset, int depth,
	const struct NodeTest * test)
{
	/* test properties if there are some */
	if (test->properties && !queryPropertyTest(fdt, offset, test->properties))
		return;
	if (test->subTest) {
		/* not a leaf in the test: recurse */
		query(fdt, offset, depth, test->subTest);
	} else {
		/* leaf of the test: action */
		printPath(fdt, offset);
	}
}

/** Query a fdt: for each node which satisfies the node test, do an action
 * \param fdt flattened device tree
 * \param offset offset to the current node
 * \param depth current node depth
 * \param test current node test
 */
static void query(const void * fdt, int offset, int depth,
	const struct NodeTest * test)
{
	switch (test->type) {
	case NODE_TEST_TYPE_ROOT:
		/* root node test is satisfied if and only if the offset points to
		 * the root node
		 */
		if (offset == 0)
			queryNode(fdt, offset, depth + 1, test);
		break;
	case NODE_TEST_TYPE_NODE:
		/* iterate over all direct sub nodes of the given node */
		offset = fdt_first_subnode(fdt, offset);
		while (offset != -FDT_ERR_NOTFOUND) {
			/* if the current node has a matching name, test the properties
			 * and maybe recurse
			 */
			if (!test->name ||
				!strcmp(test->name, fdt_get_name(fdt, offset, NULL)))
				queryNode(fdt, offset, depth + 1, test);
			offset = fdt_next_subnode(fdt, offset);
		}
		break;
	case NODE_TEST_TYPE_DESCEND: {
		/* iterate over ALL sub nodes */
		test = test->subTest; /* the test is only marked to descend and
			empty otherwise -> descend to the sub test */
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

/** Query a fdt: for each node which satisfies the node test, do an action
 * \param fdt flattened device tree
 * \param test node test
 */
void queryFdt(const void * fdt, const struct NodeTest * test)
{
	/* start at root node and depth 0 */
	query(fdt, 0, 0, test);
}

/** Print a path of a node in the device tree.
 *  This is the only action right now.
 * \param fdt flattened device tree
 * \param offset offset to node
 */
static void printPath(const void * fdt, int offset)
{
	char path[PATH_MAX];
	fdt_get_path(fdt, offset, path, sizeof path);
	printf("Node: %s @ %d: %s\n", fdt_get_name(fdt, offset, NULL), offset,
		path);
}

