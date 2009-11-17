#include "ded.h"

extern Format_s	FormatChar;
extern Format_s	FormatByte;
extern Format_s	FormatLong;
extern Format_s	FormatQuad;
extern Format_s	Format_tau_head;
extern Format_s	Format_tau_super_block;
extern Format_s	Format_tau_inode;


unint		CurrentFormat = 3;
Format_s	*Current = &FormatQuad;
Format_s	*Format[] = {
	&FormatChar,
	&FormatByte,
	&FormatLong,
	&FormatQuad,
	&Format_tau_head,
	&Format_tau_super_block,
	&Format_tau_inode
};

unint	ByteInBlock;

void next_format (void)
{
	++CurrentFormat;
	if (CurrentFormat == sizeof(Format)/sizeof(Format_s *)) {
		CurrentFormat = 0;
	}
	Current = Format[CurrentFormat];
}

void prev_format (void)
{
	if (CurrentFormat == 0) {
		CurrentFormat = sizeof(Format)/sizeof(Format_s *);
	}
	--CurrentFormat;

	Current = Format[CurrentFormat];
}

void copy (void)
{
	myError("*** no copy ***");
}

void up (void)
{
	if (ByteInBlock < u8S_PER_ROW) return;
	ByteInBlock -= u8S_PER_ROW;
}

void down (void)
{
	if (ByteInBlock >= BlockSize - u8S_PER_ROW) return;
	ByteInBlock += u8S_PER_ROW;
}

void left (void)
{
	if (ByteInBlock % u8S_PER_ROW == 0) return;
	--ByteInBlock;
}

void right (void)
{
	if (ByteInBlock % u8S_PER_ROW == u8S_PER_ROW - 1) return;
	++ByteInBlock;
}

void next (void)
{
	if (ByteInBlock >= BlockSize - 1) return;
	++ByteInBlock;
}

void prev (void)
{
	if (ByteInBlock == 0) return;
	--ByteInBlock;
}
