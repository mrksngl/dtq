
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <libfdt.h>

#include <parser.h>
#include <query.h>
#include <udt.h>

int main(int argc, char * argv[])
{
	if (argc != 3)
		error(EXIT_FAILURE, 0, "Usage: %s <filename> <query>", argv[0]);

	/* parse expression */
	struct NodeTest * query = parseNodeTestExpr(argv[2]);

	if (!query)
		return EXIT_FAILURE;

	/* debugging: print expression from AST */
	printNodeTest(query);
	puts("");

	/* open device tree */
	const char * filename = argv[1];
	int fd = open(filename, O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		error(EXIT_FAILURE, errno, "Could not open '%s'", filename);

	struct stat st;
	if (fstat(fd, &st) < 0) {
		close(fd);
		error(EXIT_FAILURE, errno, "Could not stat '%s'", filename);
	}

	/* map it into memory */
	void * fdt = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if (fdt == MAP_FAILED)
		error(EXIT_FAILURE, errno, "Could not mmap '%s'", filename);
	if (st.st_size < sizeof(struct fdt_header))
		error(EXIT_FAILURE, 0, "FDT invalid: cannot read header");

	if (fdt_totalsize(fdt) != st.st_size)
		error(EXIT_FAILURE, 0, "FDT invalid: size mismatch");


	struct DeviceTree * udt = unflattenDeviceTree(fdt);
	munmap(fdt, st.st_size);
	if (!udt)
		return EXIT_FAILURE;

	/*printDeviceTree(udt);
	freeDeviceTree(udt);*/

	/* do the query */
	queryDt(udt, query);

	/* cleanup */
	freeNodeTest(query);

	return EXIT_SUCCESS;
}
