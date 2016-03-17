
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

int main(int argc, char * argv[])
{
	if (argc != 3)
		error(EXIT_FAILURE, 0, "Usage: %s <filename> <query>", argv[0]);

	struct NavExpr * expr = parseNavExpr(argv[2]);

	if (!expr)
		return EXIT_FAILURE;

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
