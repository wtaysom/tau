/****************************************************************************
 |  (C) Copyright 2008 Novell, Inc. All Rights Reserved.
 |
 |  GPLv2: This program is free software; you can redistribute it
 |  and/or modify it under the terms of version 2 of the GNU General
 |  Public License as published by the Free Software Foundation.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 +-------------------------------------------------------------------------*/

#include <stdio.h>
#include <dis-asm.h>

#include <style.h>
#include <kmem.h>

#include <mdb.h>

typedef enum target_t { Ti386 } target_t;

target_t	Target = Ti386;

bfd_vma Vma;
struct disassemble_info Info;

static int read_memory (
	bfd_vma		address,
	bfd_byte	*data,
	unsigned	length,
	struct disassemble_info	*info)
{
	int	n;

	n = readkmem(address, data, length);
	if (n < length) return -1;
	return 0;
}

static void memory_error (
	int			status,
	bfd_vma			address,
	struct disassemble_info	*info)
{
	fprintf(info->stream, "!!! Error %d at %p\n",
		status, (void *)(addr)address);
}

static void print_address (
	bfd_vma			address,
	struct disassemble_info	*info)
{
	Sym_s	*sym;
	unint	offset;

	sym = findname(address);

	if (sym) {	//XXX: get type and module info
		if (sym_address(sym) == address) {
			fprintf(info->stream, "%s\t%p",
				sym_name(sym), (void *)(addr)address);
		} else {
			offset = address - sym_address(sym);
			fprintf(info->stream, "%s+0x%lx\t%p",
				sym_name(sym), offset, (void *)(addr)address);
		}
	} else {
		fprintf(info->stream, "%p", (void *)(addr)address);
	}
}

static int is_symbol_at_address (
	bfd_vma			address,
	struct disassemble_info	*info)
{
	Sym_s	*sym;

	sym = findname(address);

	if (sym) {
		return sym_address(sym) == address;
	} else {
		return FALSE;
	}
}

static bfd_boolean is_symbol_valid (
	asymbol	*symbolTable,
	struct disassemble_info	*info)
{
	return TRUE;
}

static void init_disasm (struct disassemble_info *info)
{
	static bool	inited = FALSE;

	if (inited) return;
	inited = TRUE;

	init_disassemble_info(info, stdout, (fprintf_ftype)fprintf);
	info->fprintf_func		= (fprintf_ftype)fprintf;
	info->stream			= stdout;
	info->application_data		= NULL;
	info->flavour			= bfd_target_unknown_flavour;
	info->arch			= bfd_arch_i386;
#if __x86_64__
	info->mach			= bfd_mach_x86_64;
#else
	info->mach			= bfd_mach_i386_i386;
#endif
	info->endian			= BFD_ENDIAN_LITTLE;
	info->insn_sets			= NULL;
	info->section			= NULL;
	info->symbols			= NULL;
	info->num_symbols		= 0;
	info->flags			= 0x0L;
	info->private_data		= NULL;
	info->read_memory_func		= read_memory;
	info->memory_error_func		= memory_error;
	info->print_address_func	= print_address;
	info->symbol_at_address_func	= is_symbol_at_address;
	info->symbol_is_valid		= is_symbol_valid;
	info->buffer			= NULL;
	info->buffer_vma		= 0;
	info->buffer_length		= 0;
	info->bytes_per_line		= 0;
	info->bytes_per_chunk		= 0;
	info->display_endian		= BFD_ENDIAN_LITTLE;
	info->octets_per_byte		= 1;
	info->skip_zeroes		= 8;
	info->skip_zeroes_at_end	= 3;
	info->disassembler_needs_relocs	= FALSE;

	/*-----------------------------------------------------------------
	** Results from instruction decoders.  Not all decoders yet support
	** this information.  This info is set each time an instruction is
	** decoded, and is only valid for the last such instruction.
	**
	** To determine whether this decoder supports this information, set
	** insn_info_valid to 0, decode an instruction, then check it.
	*/
	info->insn_info_valid		= FALSE;
	info->branch_delay_insns	= 0;
	info->data_size			= 0;
	info->insn_type			= 0;
	info->target			= 0;
	info->target2			= 0;

	info->disassembler_options	= NULL;
}

addr disasm (addr address)
{
	unint	n;

	printf("%lx:    ", address);

	init_disasm( &Info);

	switch (Target) {

	case Ti386:	n = print_insn_i386(address, &Info);
			break;
	default:
			printf("Bad target=%d\n", Target);
			break;
	}
	printf("\n");
	return n;
}
