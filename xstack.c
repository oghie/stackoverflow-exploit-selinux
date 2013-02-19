/*
 * This program rewrites an ELF header adding execute permission to the stack
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <elf.h>

int
main(int argc, const char *argv[])
{
	struct	stat	statbuf;
	Elf32_Ehdr	*ehdr;
	Elf32_Phdr	*phdr;
	char		*file;
	int		fd;
	int		i;

	/* Check command line arguments */
	if (argc != 2) {
		(void) fprintf(stderr, "Usage: %s executable\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Open for reading and writing */
	fd = open(argv[1], O_RDWR);

	/* Check that worked */
	if (fd < 0) {
		perror("open()");
		exit(EXIT_FAILURE);
	}

	/* Before we can map it, we need to know how big it is */
	if (stat(argv[1], &statbuf) < 0) {
		perror("stat()");
		exit(EXIT_FAILURE);
	}

	/*
	 * Memory map the file. Note we map it MAP_SHARED so changes to be
	 * written back to the disk.
	 */
	file = mmap(NULL, statbuf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED,
	    fd, 0);

	/* Check that worked */
	if (file == MAP_FAILED) {
		perror("mmap()");
		exit(EXIT_FAILURE);
	}

	/* Initialise pointers */
	ehdr = (Elf32_Ehdr *)file;
	phdr = (Elf32_Phdr *)(file + ehdr->e_phoff);

	/* Add execute permission to the stack */
	for (i = 0; i < ehdr->e_phnum; i++) {
		if (phdr[i].p_type == PT_GNU_STACK) {
			phdr[i].p_flags |= PF_X;
		}
	}

	/* Sync with the disk version */
	if (msync((void *)file, statbuf.st_size, MS_SYNC) < 0) {
		perror("msync()");
		exit(EXIT_FAILURE);
	}

	/* Unmap */
	(void) munmap(file, statbuf.st_size);

	/* Close the file */
	(void) close(fd);

	return (0);

}
