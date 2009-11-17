
#ifndef _TAU_DISK_H_
#define _TAU_DISK_H_ 1

/*
 *      6         5          4          3          2         1
 *  3 21098765432109 87654321098765 43210987654321 09876543210987 6543210
 * +-+--------------+--------------+--------------+--------------+-------+
 * | |    realm     |     area     |     zone     |    segment   | block |
 * +-+--------------+--------------+--------------+--------------+-------+
 *        14 bits        14 bits       14 bits        14 bits      7 bits
 *
 * Sign bit is reserved for internal use (meta data).
 */

enum {
	TAU_SEGMENT_SHIFT = 7,
	TAU_ZONE_SHIFT    = 21,
	TAU_AREA_SHIFT    = 35,
	TAU_REALM_SHIFT   = 49,

	TAU_SEGMENT_MASK = (1ULL << TAU_SEGMENT_SHIFT) - 1,

	TAU_BLOCKS_PER_SEGMENT = (1ULL << TAU_SEGMENT_SHIFT),
	TAU_SEGMENTS_PER_ZONE  = (1ULL << (TAU_ZONE_SHIFT - TAU_SEGMENT_SHIFT)),
	TAU_ZONES_PER_AREA     = (1ULL << (TAU_AREA_SHIFT - TAU_ZONE_SHIFT)),
	TAU_AREAS_PER_REALM    = (1ULL << (TAU_REALM_SHIFT - TAU_AREA_SHIFT)),

	TAU_SEGMENT_BLK = 0,
	TAU_ZONE_BLK    = 1,
	TAU_AREA_BLK    = 2,
	TAU_REALM_BLK   = 3,

	TAU_MAGIC_SEGMENT = 0x7365676d656e74ULL,
	TAU_MAGIC_ZONE    = 0x7a6f6e65656e6f7aULL,
	TAU_MAGIC_AREA    = 0x6172656161657261ULL,
	TAU_MAGIC_REALM   = 0x7265616c6dULL
};

typedef struct tau_head_s {
	__u64	h_magic;
	__u64	h_seqno;
	__u32	h_version;
	__u32	h_reserved;
} tau_head_s;

#define TAU_SUPER_HEAD(_sb) {				\
	(_sb)->sb_hd.h_magic   = TAU_MAGIC_SUPER;	\
	(_sb)->sb_hd.h_seqno   = 1;			\
	(_sb)->sb_hd.h_version = TAU_SUPER_VERSION;	\
}

#define TAU_INODE_HEAD(_ino) {				\
	(_ino)->t_hd.h_magic   = TAU_MAGIC_INODE;	\
	(_ino)->t_hd.h_seqno   = 1;			\
	(_ino)->t_hd.h_version = TAU_INO_VERSION;	\
}

#define TAU_DIR_HEAD(_dir) {				\
	(_dir)->d_hd.h_magic   = TAU_MAGIC_DIR;		\
	(_dir)->d_hd.h_seqno   = 1;			\
	(_dir)->d_hd.h_version = TAU_DIR_VERSION;	\
}

typedef struct tau_entry_s {
	__u64	e_ino;
	__u32	e_hash;
	__u32	e_reserved;
} tau_entry_s;

enum {
	TAU_SECTOR_SIZE     = 512,
	TAU_BLK_SHIFT       = 12,	/* 4096 byte block */
	TAU_BLK_SIZE        = (1 << TAU_BLK_SHIFT),
	TAU_OFFSET_MASK     = (TAU_BLK_SIZE - 1),
	TAU_SECTORS_PER_BLK = (TAU_BLK_SIZE / TAU_SECTOR_SIZE),
	TAU_BITS_PER_BLK    = 8 * TAU_BLK_SIZE,

	TAU_SUPER_BLK       = 1,
	TAU_SUPER_INO       = 1,
	TAU_ROOT_INO        = 1 + TAU_SUPER_INO,
	TAU_START_FREE      = 1 + TAU_ROOT_INO,

	TAU_NAME_LEN        = 255,	// *3?

	TAU_INOS_PER_BLK    = (TAU_BLK_SIZE / sizeof(u64)),
	TAU_INDIRECT_SHIFT  = 8,
	TAU_NUM_DIRECT      = (1 << 8),
	TAU_NUM_INDIRECT    = (1 << TAU_INDIRECT_SHIFT),
	TAU_INDIRECT_MASK   = (TAU_NUM_INDIRECT - 1),
	TAU_MAX_LEVELS      = 6,
	TAU_ENTRIES_PER_BLK = ((TAU_BLK_SIZE - sizeof(tau_head_s))
					/ sizeof(tau_entry_s)),
	TAU_MAX_FILE_SIZE   = TAU_NUM_DIRECT * TAU_BLK_SIZE,
	TAU_CLEAR_ENDS      = 16,	// Num blocks cleared at ends of fs
	TAU_MIN_BLOCKS      = TAU_CLEAR_ENDS,	// Min fs size

	TAU_SUPER_VERSION   = 1,
	TAU_INO_VERSION     = 1,
	TAU_DIR_VERSION     = 1,
	TAU_IND_VERSION     = 1,
	TAU_MAGIC_SUPER     = 0x7265707573756174ULL,	// tausuper
	TAU_MAGIC_INODE     = 0x65646f6e69756174ULL,	// tauinode
	TAU_MAGIC_DIR       = 0x726964,
	TAU_MAGIC_IND       = 0x696e64
};

typedef struct tau_super_block_s
{
	tau_head_s	sb_hd;
	__u64		sb_numblocks;
	__u64		sb_nextblock;
	__u32		sb_blocksize;
} tau_super_block_s;

/*
 * For the ext2 file system, there are three structures that share field
 * names with the same name -- inode, ext2_inode, ext2_info_inode.  I
 * would prefer to use names unique to each structure.
 *	inode - defined across file systems.
 *	tau_inode - raw inode on disk format
 *	tau_info_inode - full in memory version of inode
 */

/*
 *      6          5          4           3          2          1
 *  3 210987654321 098 76543210 98765432 10987654 32109876 54321098 76543210
 * +-+------------+---+--------+--------+--------+--------+--------+--------+
 * | | meta-data  |hex| quint  |  quad  | triple | double |indirect| direct |
 * +-+------------+---+--------+--------+--------+--------+--------+--------+
 *
 * Sign bit is reserved.
 * The POSIX lseek api limits us to 8 Exabytes.  So we can use bits 51 through
 * 62 to augment logical addresses for meta-data blocks.
 */

enum {
	TAU_DIRECT   = (1 << 8),
	TAU_INDIRECT = (1 << 16),
	TAU_DOUBLE   = (1 << 24),
	TAU_TRIPLE   = (1LL << 32),
	TAU_QUAD     = (1LL << 40)
};

typedef struct tau_inode_s {
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
} tau_inode_s;

typedef struct tau_indirect_s {
	tau_head_s	in_hd;
	u64		in_blocks[TAU_NUM_INDIRECT];
} tau_indirect_s;

typedef struct tau_dir_s {
	tau_head_s	d_hd;
	tau_entry_s	d_entry[TAU_ENTRIES_PER_BLK];
} tau_dir_s;

typedef struct tau_blk_s {
	__u64	b_inode;
	__u64	b_logical;
} tau_blk_s;


typedef struct tau_segment_s {
	union {
		struct {
			tau_head_s	sg_hd;
			__u64		sg_free[TAU_BLOCKS_PER_SEGMENT / 64];
			__u64		sg_state[TAU_BLOCKS_PER_SEGMENT / 64];
		} hd;
		__u8		sg_space[TAU_BLK_SIZE / 2];
	} sg;
	tau_blk_s	sg_blk[TAU_BLOCKS_PER_SEGMENT];
} tau_segment_s;

typedef struct tau_zone_s {
	union {
		struct {
			tau_head_s	z_hd;
		} hd;
		__u8		z_space[TAU_BLK_SIZE / 2];
	} z;
	__u64		z_free[TAU_SEGMENTS_PER_ZONE / 64];
} tau_zone_s;

typedef struct tau_area_s {
	union {
		struct {
			tau_head_s	a_hd;
		} hd;
		__u8		a_space[TAU_BLK_SIZE / 2];
	} sg;
	__u64		a_free[TAU_ZONES_PER_AREA / 64];
} tau_area_s;

typedef struct tau_realm_s {
	union {
		struct {
			tau_head_s	r_hd;
		} hd;
		__u8		r_space[TAU_BLK_SIZE / 2];
	} sg;
	__u64		r_free[TAU_AREAS_PER_REALM / 64];
} tau_realm_s;


typedef struct key_s {
	__u64	k_key;
	__u64	k_block;
} key_s;

typedef struct rec_s {
	__s16	r_start;
	__s16	r_size;
	__s32	r_reserverd;
	__u64	r_key;
} rec_s;

struct tree_s {
	__u16	t_type;
	__u16	t_reserved[3];
	__u64	t_magic;
	__u64	t_root;
	__u64	t_next;
};

typedef struct block_s {
	__u16	b_type;
	__u16	b_reserved;
} block_s;

/* b0 k1 b1 k2 b2 k3 b3 */
typedef struct branch_s {
	__u16	br_type;
	__s16	br_num;
	__u16	br_reserved[2];
	__u64	br_first;
	key_s	br_key[0];
} branch_s;

typedef struct leaf_s {
	__u16	l_type;
	__s16	l_num;
	__s16	l_end;
	__s16	l_total;	// change to free later
	// Another way to make end work would be to use
	// &l_rec[l_num] as the index point of l_end
	// That is what I did in insert!!!
	rec_s	l_rec[0];
} leaf_s;


#endif
