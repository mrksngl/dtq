
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

static off_t getSubNode(const void * fdt, off_t offset, int depth,
	const struct NavExpr * expr)
{
	int cdepth = depth;

	do {
		offset = fdt_next_node(fdt, offset, &cdepth);

		if (cdepth == depth + 1) {
			const char * name = fdt_get_name(fdt, offset, NULL);
			printf("%s <> %s\n", expr->name, name);
			if (!strcmp(expr->name, name))
				return offset;
		}
	} while (cdepth >= 0);

	return -FDT_ERR_NOTFOUND;
}

static off_t getNode(const void * fdt, off_t offset, int depth,
	const struct NavExpr * expr, off_t * lastNode)
{
	offset = getSubNode(fdt, offset, depth, expr);
	if (offset == -FDT_ERR_NOTFOUND)
		return offset;
	*lastNode = offset;
	if (expr->subExpr)
		offset = getNode(fdt, offset, depth + 1, expr->subExpr, lastNode);
	return offset;
}

static void queryFdt(const void * fdt, const struct NavExpr * expr)
{
	/* start at root node */
	off_t lastNode = 0;

	int off2 = getNode(fdt, 0, 0, expr, &lastNode);

	if (off2 == -FDT_ERR_NOTFOUND) {
		printf("Not found after "); printPath(fdt, lastNode);
	} else {
		printPath(fdt, off2);
	}
}

static void printPath(const void * fdt, off_t offset)
{
	char path[PATH_MAX];
	fdt_get_path(fdt, offset, path, sizeof path);
	printf("Node: %s @ %ld: %s\n", fdt_get_name(fdt, offset, NULL), offset, path);
}

