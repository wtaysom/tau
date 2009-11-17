#include <linux/time.h>
#include <stddef.h>

#include <style.h>
#include <tau_disk.h>

#include "ded.h"


#define TIMEVAL(_f, _t)	FIELD(struct timeval, _f, _t, NULL)

#define sTIMEVAL(_f)	TIMEVAL(_f, fSTRING)
#define iTIMEVAL(_f)	TIMEVAL(_f, fINT)
#define gTIMEVAL(_f)	TIMEVAL(_f, fGUID)

static Field_s Timeval[] = {
	iTIMEVAL(tv_sec),
	iTIMEVAL(tv_usec),
	END_FIELD};

static void timeval_display (void)
{
	display_fields(Timeval);
}

Format_s Fromat_timeval = {
	"timeval",
	timeval_display, copy_field, left_field, right_field,
	up_field, down_field, next_field, prev_field};



#define HEAD(_f, _t)	FIELD(tau_head_s, _f, _t, NULL)

#define sHEAD(_f)	HEAD(_f, fSTRING)
#define iHEAD(_f)	HEAD(_f, fINT)
#define gHEAD(_f)	HEAD(_f, fGUID)

static Field_s Head[] = {
	iHEAD(h_magic),
	iHEAD(h_seqno),
	iHEAD(h_version),
	iHEAD(h_reserved),
	END_FIELD};

static void head_display (void)
{
	display_fields(Head);
}

Format_s Format_tau_head = {
	"tau head",
	head_display, copy_field, left_field, right_field,
	up_field, down_field, next_field, prev_field};



#define SUPER(_f, _t)	FIELD(tau_super_block_s, _f, _t, NULL)

#define sSUPER(_f)	SUPER(_f, fSTRING)
#define iSUPER(_f)	SUPER(_f, fINT)
#define gSUPER(_f)	SUPER(_f, fGUID)
#define fSUPER(_n, _f)	{ MAGIC_STRING(_n), fSTRUCT, 0, 0, _f }

static Field_s Super_block[] = {
	fSUPER(sb_hd, Head),
	iSUPER(sb_numblocks),
	iSUPER(sb_nextblock),
	iSUPER(sb_blocksize),
	END_FIELD};

static void super_block_display (void)
{
	display_fields(Super_block);
}

Format_s Format_tau_super_block = {
	"tau super block",
	super_block_display, copy_field, left_field, right_field,
	up_field, down_field, next_field, prev_field
};



#define INODE(_f, _t)	FIELD(tau_inode_s, _f, _t, NULL)

#define sINODE(_f)	INODE(_f, fSTRING)
#define iINODE(_f)	INODE(_f, fINT)
#define gINODE(_f)	INODE(_f, fGUID)
#define fINODE(_n, _f)	{ MAGIC_STRING(_n), fSTRUCT, 0, 0, _f }

#if 0
	tau_head_s	t_hd;
	__u64		t_ino;
	__u64		t_parent;	/* Parent inode */
	__u16		t_mode;
	__u16		t_link;		/* Reserved for future Evil */
	__u32		t_blksize;
	__u64		t_size;		/* Logical size of file (EOF) */
	__u64		t_blocks;	/* Total blocks assigned to file */
	__u32		t_uid;
	__u32		t_gid;
	struct timespec	t_atime;
	struct timespec	t_mtime;
	struct timespec	t_ctime;
	char		t_name[TAU_NAME_LEN+1];
	__u64		t_direct[TAU_NUM_DIRECT];
	__u64		t_indirect[TAU_MAX_LEVELS];
#endif

static Field_s Inode[] = {
	fINODE(t_head, Head),
	iINODE(t_ino),
	iINODE(t_parent),
	iINODE(t_mode),
	iINODE(t_link),
	iINODE(t_blksize),
	iINODE(t_size),
	iINODE(t_blocks),
	iINODE(t_uid),
	iINODE(t_gid),
	fINODE(t_atime, Timeval),
	fINODE(t_mtime, Timeval),
	fINODE(t_ctime, Timeval),
	END_FIELD};

static void inode_display (void)
{
	display_fields(Inode);
}

Format_s Format_tau_inode = {
	"tau inode",
	inode_display, copy_field, left_field, right_field,
	up_field, down_field, next_field, prev_field
};
