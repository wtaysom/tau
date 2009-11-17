#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <eprintf.h>
#include <style.h>
#include "ded.h"

int	Disk;
char	*DiskName;
u8	*Block;
u64	BlockNum = ~0;
unint	BlockSize = MAX_BLOCK_SIZE;

int initDisk (char *device)
{
	Block = emalloc(BlockSize);

	DiskName = device;
	Disk = open(DiskName, O_RDONLY);
	if (Disk == -1) {
		sysError(DiskName);
		return -1;
	}
	return 0;
}

int readDisk (u64 new_block)
{
	off_t	offset;
	ssize_t	bytesRead;

	offset = new_block * BlockSize;
	if (lseek(Disk, offset, 0) == -1) {
		sysError("lseek");
		return -1;
	}
	bytesRead = read(Disk, Block, BlockSize);
	if (bytesRead == 0) {
		myError("At EOF");
		return 0;
	}
	if (bytesRead == -1) {
		sysError("read");
		return -1;
	}
	if (bytesRead < BlockSize) {
		bzero( &Block[bytesRead], BlockSize - bytesRead);
	}
	BlockNum = new_block;
	return 0;
}
