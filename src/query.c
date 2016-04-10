
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
#include <udt.h>

static void printPath(const struct Node * node);
static void query(const struct Node * node, const struct NodeTest * test);

static bool containsString(const struct Property * prop, const char * str)
{
	const char * data = prop->val;
	const char * end = data + prop->len;

	do {
		if (!strcmp(data, str))
			return true;
		data += strlen(data) + 1;
	} while (data < end);
	return false;
}

static bool containsInt(const struct Property * prop, uint32_t i)
{
	const char * data = prop->val;
	const char * end = data + prop->len;

	while (data + 3 < end) {
		if (fdt32_to_cpu(*(fdt32_t*)data) == i)
			return true;
		data += 4;
	}
	return false;
}

static bool queryAtomicPropertyTest(const struct Node * node,
	const struct AtomicPropertyTest * test)
{
	const struct Property * prop;
	for (prop = node->properties; prop; prop = prop->nextProperty)
		if (!strcmp(test->property, prop->name))
			break;
	if (!prop)
		return false;

	switch (test->type) {
	case ATOMIC_PROPERTY_TEST_TYPE_EXIST:
		return true;
	case ATOMIC_PROPERTY_TEST_TYPE_INT:
		if (test->op == ATOMIC_PROPERTY_TEST_OP_CONTAINS) {
			return containsInt(prop, test->integer);
		} else {
			if (prop->len != 4)
				return false;
			uint32_t i = fdt32_to_cpu(*(fdt32_t*)prop->val);

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
			return containsString(prop, test->string);
		} else {
			bool res = prop->len == strlen(test->string) + 1;
			res = res && !memcmp(test->string, prop->val, prop->len);
			return !(res ^ (test->op == ATOMIC_PROPERTY_TEST_OP_EQ));
		}
		break;
	default:
		assert(false);
	}
	return false;
}

static bool queryPropertyTest(const struct Node * node,
	const struct PropertyTest * test)
{
	switch (test->type) {
	case PROPERTY_TEST_OP_AND:
		return queryPropertyTest(node, test->left) &&
			queryPropertyTest(node, test->right);
	case PROPERTY_TEST_OP_OR:
		return queryPropertyTest(node, test->left) ||
			queryPropertyTest(node, test->right);
	case PROPERTY_TEST_OP_NEG:
		return !queryPropertyTest(node, test->child);
	case PROPERTY_TEST_OP_ATOMIC:
		return queryAtomicPropertyTest(node, test->atomic);
	default:
		return false;
	}
}

/** Test if a node matches a query
 * \param node node to be tested
 * \param test test to be queried on the node
 * \return true if node matches the test
 */
static bool testNode(const struct Node * node, const struct NodeTest * test)
{
	switch (test->type) {
	case NODE_TEST_TYPE_ROOT:
		if (node->parent != NULL)
			return false;
		break;
	case NODE_TEST_TYPE_NODE:
		if (test->name && strcmp(test->name, node->name))
			return false;
		break;
	case NODE_TEST_TYPE_DESCEND:
		assert(false);
		break;
	}

	if (test->properties)
		return queryPropertyTest(node, test->properties);
	else
		return true;
}

/** Descend on a query: test all successor nodes
 * \param node root node of the test: all its successor nodes will be queried
 * \param test test to apply to the nodes
 */
static void queryDescend(const struct Node * node, const struct NodeTest * test)
{
	const struct Node * child;
	for (child = node->children; child; child = child->sibling) {
		query(child, test);
		queryDescend(child, test);
	}
}

/** Query a dt: test a node, execute action, recurse or do nothing
 * \param node node to be tested
 * \param test node test
 */
static void query(const struct Node * node, const struct NodeTest * test)
{
	if (!testNode(node, test))
		return;

	/* test passed, consider children */
	if (!test->subTest) {
		/* leaf of the test: action */
		printPath(node);
		return;
	}

	/* descend */
	const struct NodeTest * subTest = test->subTest;
	if (subTest->type == NODE_TEST_TYPE_DESCEND) {
		queryDescend(node, subTest->subTest);
	} else {
		struct Node * child;
		for (child = node->children; child; child = child->sibling)
			query(child, subTest);
	}
}

/** Query a dt: for each node which satisfies the node test, execute an action
 * \param dt unflattened device tree
 * \param test node test
 */
void queryDt(const struct DeviceTree * dt, const struct NodeTest * test)
{
	/* start at root node */
	query(dt->root, test);
}

static void printPathName(const struct Node * node)
{
	if (node->parent) {
		printPathName(node->parent);
		printf("/%s", node->name);
	}
}

/** Print a path of a node in the device tree.
 *  This is the only action right now.
 * \param fdt flattened device tree
 * \param offset offset to node
 */
static void printPath(const struct Node * node)
{
	printf("Node: %s: ", node->name);
	printPathName(node);
	putchar('\n');
}

