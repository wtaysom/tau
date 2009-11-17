#ifndef _DED_H_
#define _DED_H_

#ifndef _STYLE_H_
#include <style.h>
#endif

typedef enum Radix_t {
	HEX, UNSIGNED, SIGNED, CHARACTER
} Radix_t;

/*
 * A block is the unit of storage we use in the file system.
 * A sector is how much we can display at a time.
 */
enum {
	STK_SIZE	= 4,
	MAX_MEM		= 4,
	MAX_BLOCK_SIZE	= 4096,
	DIGITS_PER_u8 = 2,

	MAX_ROW		= 80,
	u8S_PER_ROW	= 64,
	START_ROW	= 2,
	START_COL	= 4,
	HELP_ROW	= 0,
	HELP_COL	= 0,
	HELP_SIZE	= 25,
	ERR_ROW		= 0,
	ERR_COL		= 60,
	STK_COL		= 40,

	MAX_VALUE_SIZE	= sizeof(u64) * DIGITS_PER_u8
};

typedef enum field_t { fINT, fSTRING, fGUID, fSTRUCT } field_t;

typedef struct Field_s	Field_s;
struct Field_s {
	char		*f_name;
	field_t		f_type;
	unsigned	f_size;
	unsigned	f_offset;
	Field_s		*f_subfields;
};

#define FIELD(_struct, _field, _type, _subfields)			\
	{ MAGIC_STRING(_field), _type, sizeof(((_struct *)0)->_field),	\
		offsetof(_struct, _field), _subfields }

#define END_FIELD	{ NULL, 0, 0, 0 }

#ifndef _STDIO_H_
#include <stdio.h>
#endif

#ifdef PJT

#define PR(_x)	(fprintf(stderr, "%s\n", # _x))
#define PRs(_x)	(fprintf(stderr, "%s=%s\n", # _x, _x))
#define PRx(_x)	(fprintf(stderr, "%s=%x\n", # _x, _x))
#define PRq(_x)	(fprintf(stderr, "%s=%qx\n", # _x, _x))
#define PRd(_x)	(fprintf(stderr, "%s=%d\n", # _x, _x))
#define PRu(_x)	(fprintf(stderr, "%s=%u\n", # _x, _x))
#define PRp(_x)	(fprintf(stderr, "%s=%p\n", # _x, _x))

#else

#define PR(_x)	((void) 0)
#define PRs(_x)	((void) 0)
#define PRx(_x)	((void) 0)
#define PRd(_x)	((void) 0)
#define PRu(_x)	((void) 0)
#define PRp(_x)	((void) 0)

#endif

typedef struct Format_s {
	char	*title;
	void	(*display)(void);
	void	(*copy)(void);
	void	(*left)(void);
	void	(*right)(void);
	void	(*up)(void);
	void	(*down)(void);
	void	(*next)(void);
	void	(*prev)(void);
} Format_s;

extern char	Error[];
extern u8	*Block;
extern unint	BlockSize;
extern unint	ByteInBlock;
extern u64	BlockNum;
extern u64	NextBlock;
extern bool	InputMode;
extern u64	Lastx;
extern bool	NewNumber;
extern u64	Memory[];
extern Radix_t	Radix;
extern Format_s	*Current;
extern int	NumRows;

void finish  (int sig);
int sysError (char *s);
int myError  (char *s);

int initDisk (char *device);
int readDisk (u64 new_sector);

void input (void);

void next_format (void);
void prev_format (void);
void output      (void);
void help        (void);

int  max_name_size  (Field_s *fields);
int  max_value_size (Field_s *fields);
void dump_fields    (Field_s *fields);
void display_fields (Field_s *fields);

void copy_field  (void);
void up_field    (void);
void down_field  (void);
void left_field  (void);
void right_field (void);
void next_field  (void);
void prev_field  (void);


void initStack (void);
u64  peek   (void);
u64  pop    (void);
void push   (u64);
void swap   (void);
void rotate (void);
void duptop (void);
u64 ith     (unint);

void initMemory (void);
void store      (void);
void retrieve   (void);

void add    (void);
void sub    (void);
void mul    (void);
void divide (void);
void mod    (void);
void neg    (void);
void not    (void);
void and    (void);
void or     (void);
void xor    (void);
void leftShift  (void);
void rightShift (void);
void qrand  (void);

#endif
