
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <error.h>
#include <assert.h>
#include <libfdt.h>
#include <limits.h>

#include <dtq-parse.h>

static void queryFdt(const void * fdt, const struct NavExpr * expr);

int main(int argc, char * argv[])
{
	if (argc != 3)
		error(EXIT_FAILURE, 0, "Usage: %s <filename> <query>", argv[0]);

	struct NavExpr * expr = parseNavExpr(argv[2]);

	printNavExpr(expr);
	printf("\n");

	const char * filename = argv[1];
	int fd = open(filename, O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		error(EXIT_FAILURE, errno, "Could not open '%s'", filename);

	struct stat st;
	if (fstat(fd, &st) < 0) {
		close(fd);
		error(EXIT_FAILURE, errno, "Could not stat '%s'", filename);
	}

	void * fdt = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if (fdt == MAP_FAILED)
		error(EXIT_FAILURE, errno, "Could not mmap '%s'", filename);
	if (st.st_size < sizeof(struct fdt_header))
		error(EXIT_FAILURE, 0, "FDT invalid: cannot read header");

	if (fdt_totalsize(fdt) != st.st_size)
		error(EXIT_FAILURE, 0, "FDT invalid: size mismatch");

	queryFdt(fdt, expr);

	freeNavExpr(expr);

	munmap(fdt, st.st_size);
	return EXIT_SUCCESS;
}

static void printPath(const void * fdt, off_t offset);

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

static bool testTest(const void * fdt, off_t offset,
	const struct TestExpr * test)
{
	int len;
	const struct fdt_property * prop = fdt_get_property(fdt, offset,
		test->property, &len);
	if (!prop)
		return false;

	if (test->type == TEST_TYPE_EXIST)
		return true;

	enum TEST_TYPE operator = test->type & TEST_TYPE_MASK;

	if (test->type & TEST_TYPE_STR) {
		if (operator == TEST_TYPE_CONTAINS) {
			return containsString(prop->data, len, test->string);
		} else {
			bool res = len == strlen(test->string) + 1;
			res = res && !memcmp(test->string, prop->data, len);
			return !(res ^ (operator == TEST_TYPE_EQ));
		}
	} else {
		if (operator == TEST_TYPE_CONTAINS) {
			return containsInt(prop->data, len, test->integer);
		} else {
			if (len != 4)
				return false;
			uint32_t i = fdt32_to_cpu(*(fdt32_t*)prop->data);

			switch (test->type & TEST_TYPE_MASK) {
			case TEST_TYPE_EQ: return i == test->integer;
			case TEST_TYPE_NE: return i != test->integer;
			case TEST_TYPE_LE: return i < test->integer;
			case TEST_TYPE_GE: return i > test->integer;
			case TEST_TYPE_LT: return i <= test->integer;
			case TEST_TYPE_GT: return i >= test->integer;
			default: return false;
			}
		}
	}
}

static bool testAttributes(const void * fdt, off_t offset,
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

static void evaluate(const void * fdt, off_t offset, int depth,
	const struct NavExpr * expr)
{
	int cdepth = depth;

	while (true) {
		offset = fdt_next_node(fdt, offset, &cdepth);
		if (cdepth == 0)
			break;
		if (expr->name) {
			if (cdepth != depth + 1)
				continue;
			const char * name = fdt_get_name(fdt, offset, NULL);
			//printf("%s <> %s\n", expr->name, name);
			if (strcmp(expr->name, name))
				continue;
		} else if (cdepth <= depth)
			continue;
		if (expr->attributes && !testAttributes(fdt, offset, expr->attributes))
			continue;
		if (expr->subExpr) {
			evaluate(fdt, offset, cdepth, expr->subExpr);
		} else {
			printPath(fdt, offset);
		}
	}
}

static void queryFdt(const void * fdt, const struct NavExpr * expr)
{
	evaluate(fdt, 0, 0, expr);
}

static void printPath(const void * fdt, off_t offset)
{
	char path[PATH_MAX];
	fdt_get_path(fdt, offset, path, sizeof path);
	printf("Node: %s @ %ld: %s\n", fdt_get_name(fdt, offset, NULL), offset, path);
}

