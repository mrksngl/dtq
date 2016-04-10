
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <udt.h>
#include <libfdt.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

static void unflattenNode(struct Node ** ret, struct Node * parent,
	const void * fdt, int * offset, int * depth)
{
	struct Node * node = malloc(sizeof *node);
	assert(node);
	*ret = node;
	if (*offset) {
		const char * name = fdt_get_name(fdt, *offset, NULL);
		if (name)
			node->name = strdup(name);
		else
			node->name = NULL;
	} else node->name = NULL;
	node->parent = parent;
	node->children = NULL;
	node->sibling = NULL;

	struct Property ** pprop = &node->properties;
	int propoff = fdt_first_property_offset(fdt, *offset);
	while (propoff != -FDT_ERR_NOTFOUND) {
		struct Property * prop = malloc(sizeof *prop);
		const char * name;
		const void * val = fdt_getprop_by_offset(fdt, propoff, &name,
			&prop->len);
		prop->name = strdup(name);
		prop->val = malloc(prop->len);
		memcpy(prop->val, val, prop->len);
		*pprop = prop;
		pprop = &prop->nextProperty;
		propoff = fdt_next_property_offset(fdt, propoff);
	}
	*pprop = NULL;

	int mydepth = *depth;
	*offset = fdt_next_node(fdt, *offset, depth);

	if (*offset < 1 || *depth < mydepth)
		return;

	if (*depth == mydepth + 1)
		unflattenNode(&node->children, node, fdt, offset, depth);
	if (*depth == mydepth)
		unflattenNode(&node->sibling, parent, fdt, offset, depth);
	//assert(*depth < mydepth);
}

struct DeviceTree * unflattenDeviceTree(const void * fdt)
{
	struct DeviceTree * dt = malloc(sizeof *dt);
	assert(dt);
	dt->boot_cpuid_phys = fdt_boot_cpuid_phys(fdt);
	int offset = 0;
	int depth = 0;
	unflattenNode(&dt->root, NULL, fdt, &offset, &depth);

	return dt;
}

static const char spaces[10] = { [0 ... 9] = ' ' };

static void printNode(const struct Node * node, int level)
{
	int l;
	for (l = level << 1; l > 10; l -= 10)
		fwrite(spaces, sizeof *spaces, sizeof spaces, stdout);
	if (l)
		fwrite(spaces, sizeof *spaces, l, stdout);
	if (node->parent == NULL)
		printf("(root)\n");
	else
		printf("%s\n", node->name ?: "(null)");
	if (node->children)
		printNode(node->children, level + 1);
	if (node->sibling)
		printNode(node->sibling, level);
}

void printDeviceTree(const struct DeviceTree * dt)
{
	printNode(dt->root, 0);
}

static void freeNode(struct Node * node)
{
	if (!node)
		return;

	free(node->name);
	struct Property * prop, * nprop;
	for (prop = node->properties; prop; prop = nprop) {
		nprop = prop->nextProperty;
		free(prop->name);
		free(prop->val);
		free(prop);
	}
	freeNode(node->children);
	freeNode(node->sibling);

	free(node);
}

void freeDeviceTree(struct DeviceTree * tree)
{
	freeNode(tree->root);
	free(tree);
}
