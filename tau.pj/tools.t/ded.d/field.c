#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <ncurses.h>

#include "ded.h"

static Field_s	*Fields;

void dump_field (Field_s *f)
{
	fprintf(stderr, "%-20s %2d %3d %4d\n",
			f->f_name, f->f_type, f->f_size, f->f_offset);
}

void dump_fields (Field_s *fields)
{
	Field_s	*f;

	if (!fields) return;
	fprintf(stderr, "name               type size offset\n");
	for (f = fields; f->f_name; f++) {
		dump_field(f);
	}
}

int next_offset_subfield (Field_s *fields, int offset, int sum)
{
	Field_s	*f;

	if (!fields) return sum;

	for (f = fields; f->f_name; f++) {
		if (sum > offset) {
			return sum;
		}
		sum = next_offset_subfield(f->f_subfields, offset, sum + f->f_size);
	}
	return sum;
}

int next_offset (Field_s *fields, int offset)
{
	return next_offset_subfield(fields, offset, 0);
}

enum { DONE = 0, CONT = 1 };
int prev_offset_subfield (Field_s *fields, int offset, int old, int *sum)
{
	Field_s	*f;

	if (!fields) return CONT;

	for (f = fields; f->f_name; f++) {
		if (*sum >= offset) {
			break;
		}
		old = *sum;
		*sum += f->f_size;
		if (!prev_offset_subfield(f->f_subfields, offset, old, sum)) {
			return DONE;
		}
	}
	if (*sum >= offset) {
		*sum = old;
		return DONE;
	}
	return CONT;
}

int prev_offset (Field_s *fields, int offset)
{
	int	sum = 0;

	prev_offset_subfield(fields, offset, 0, &sum);
	return sum;
}


int max_name_size (Field_s *fields)
{
	Field_s	*f;
	int	max = 0;
	int	n;

	if (!fields) return 0;

	for (f = fields; f->f_name; f++) {
		n = strlen(f->f_name);
		if (n > max) {
			max = n;
		}
		n = max_name_size(f->f_subfields);
		if (n > max) {
			max = n;
		}
	}
	return max;
}

int max_value_size (Field_s *fields)
{
	Field_s	*f;
	int	max = 0;
	int	n;

	if (!fields) return 0;

	for (f = fields; f->f_name; f++) {
		switch (f->f_type) {
		case fINT:
			n = f->f_size * DIGITS_PER_u8;
			break;
		case fSTRING:
			n = f->f_size;
			break;
		case fGUID:
			n = f->f_size / 2;
			break;
		case fSTRUCT:	// This could be on any field
			n = max_value_size(f->f_subfields);
			break;
		default:
			n = 0;
			fprintf(stderr, "Bad field %s\n", f->f_name);
			finish(0);
			break;
		}
		if (n > MAX_VALUE_SIZE) {
			n = MAX_VALUE_SIZE;
		}
		if (n > max) {
			max = n;
		}
	}
	return max;
}

static int	Max_name;
static int	Max_value;
static int	Row;
static int	Col;
static int	Col_width;
static int	Value_col;
static u8	*Cp;

static void init_display (Field_s *fields)
{
	Fields    = fields;

	Max_name  = max_name_size(fields);
	Max_value = max_value_size(fields);
	Col_width = Max_name + Max_value + 2;

	Cp = Block;
	Row = START_ROW;
	Col = 0;
	Value_col = Col + Max_name + 1;
}

static void display_string (int row, int col, u8 *s, int n)
{
	int	i;

	for (i = 0; i < n; s++, col++, i++) {

		mvprintw(row, col, "%c", isgraph(*s) ? *s : '.');
	}
}

static void display (Field_s *fields)
{
	Field_s	*f;
	int	highlight = FALSE;

	for (f = fields; f->f_name; f++) {
		mvprintw(Row, Col, "%.*s ", Max_name, f->f_name);

		if (f->f_size && ((Cp - Block) <= ByteInBlock)
				&& (ByteInBlock < ((Cp - Block) + f->f_size)))
		{
			attron(A_REVERSE);
			highlight = TRUE;
		}
		switch (f->f_type) {
		case fINT:
			switch (f->f_size) {
			case 1:
				mvprintw(Row, Value_col, "%*x",
						Max_value, *(u8 *)Cp);
				break;
			case 2:
				mvprintw(Row, Value_col, "%*x",
						Max_value, *(u16 *)Cp);
				break;
			case 4:
				mvprintw(Row, Value_col, "%*lx",
						Max_value, *(u32 *)Cp);
				break;
			case 8:
				mvprintw(Row, Value_col, "%*llx",
						Max_value, *(u64 *)Cp);
				break;
			default:
				mvprintw(Row, Value_col, "field size %3d",
						f->f_size);
				break;
			}
			break;

		case fSTRING:
			display_string(Row, Value_col, Cp,
				(Max_value < f->f_size) ? Max_value : f->f_size);
			break;
		case fGUID:
			mvprintw(Row, Value_col, "         GUID");
			break;
		case fSTRUCT:
			display(f->f_subfields);
			break;
		default:
			fprintf(stderr, "Bad field %s\n", f->f_name);
			break;
		}
		if (highlight) {
			attroff(A_REVERSE);
			highlight = FALSE;
		}
		Cp += f->f_size;
		if (++Row == START_ROW + NumRows) {
			Row = START_ROW;
			Col += Col_width;
			Value_col = Col + Max_name + 1;
		}
	}
}

void display_fields (Field_s *fields)
{
	init_display(fields);
	display(fields);
}

void copy_field (void)
{
	myError("*** no copy ***");
}

void next_field (void)
{
	if (ByteInBlock >= BlockSize - 1) return;
	ByteInBlock = next_offset(Fields, ByteInBlock);
}

void prev_field (void)
{
	if (ByteInBlock == 0) return;
	ByteInBlock = prev_offset(Fields, ByteInBlock);
}

void up_field (void)
{
	prev_field();
}

void down_field (void)
{
	next_field();
}

void left_field (void)
{
	prev_field();
}

void right_field (void)
{
	next_field();
}
