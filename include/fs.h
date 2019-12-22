#ifndef _FS_H_
#define _FS_H_
#include "type.h"
#include "hd.h"

/* major device numbers (corresponding to kernel/global.c::dd_map[]) */
#define	NO_DEV			0
#define	DEV_FLOPPY		1
#define	DEV_CDROM		2
#define	DEV_HD			3
#define	DEV_CHAR_TTY	4
#define	DEV_SCSI		5

/* make device number from major and minor numbers */
#define	MAJOR_SHIFT		8
#define	MAKE_DEV(a,b)		((a << MAJOR_SHIFT) | b)

/* separate major and minor numbers from device number */
#define	MAJOR(x)		((x >> MAJOR_SHIFT) & 0xFF)
#define	MINOR(x)		(x & 0xFF)

struct DevDrvMap
{
	int driverNr; /**< The proc nr.\ of the device driver. */
};

/* 超级块 */
struct SuperBlock
{
	u32	magic;		        /**< Magic number*/
	u32	nrInodes;	        /**< How many inodes*/
	u32	nrSects;	        /**< How many sectors*/
	u32	nrImapSects;	    /**< How many inode-map sectors*/
	u32	nrSmapSects;	    /**< How many sector-map sectors*/
	u32	n1stSect;	        /**< Number of the 1st data sector*/
	u32	nrInodeSects;       /**< How many inode sectors*/
	u32	rootInode;          /**< Inode nr of root directory*/
	u32	inodeSize;          /**< INODE_SIZE */
	u32	inodeISizeOff;      /**< Offset of `struct inode::i_size' */
	u32	inodeStartOff;      /**< Offset of `struct inode::i_start_sect' */
	u32	dirEntSize;         /**< DIR_ENTRY_SIZE */
	u32	dirEntInodeOff;     /**< Offset of `struct dir_entry::inode_nr' */
	u32	dirEntFnameOff;     /**< Offset of `struct dir_entry::name' */

	/*
	 * the following item(s) are only present in memory
	 */
	int	sbDev; 	/**< the super block's home device */
};

#define	SUPER_BLOCK_SIZE	56

struct Inode
{
	u32	iMode;		    /**< Accsess mode */
	u32	iSize;		    /**< File size */
	u32	iStartSect;	    /**< The first sector of the data */
	u32	iNrSects;	    /**< How many sectors the file occupies */
	u8	_unused[16];	/**< Stuff for alignment */

	/* the following items are only present in memory */
	int	iDev;
	int	iCnt;		    /**< How many procs share this inode  */
	int	iNum;		    /**< inode nr.  */
};

#define	INODE_SIZE	32
#define	MAX_FILENAME_LEN	12

struct DirEntry
{
	int	inodeNr;		            /**< inode nr. */
	char name[MAX_FILENAME_LEN];	/**< Filename */
};

#define	MAGIC_V1	0x111

#define	INVALID_INODE		0
#define	ROOT_INODE		    1

#define	DIR_ENTRY_SIZE	sizeof(struct DirEntry)

void taskFs();

#define RD_SECT(dev,sectNr) rwSector(DEV_READ, \
                				       dev,				\
                				       (sectNr) * SECTOR_SIZE,		\
                				       SECTOR_SIZE, /* read one sector */ \
                				       P_TASK_FS,				\
                				       fsbuf);

#define WR_SECT(dev,sectNr) rwSector(DEV_WRITE, \
                				       dev,				\
                				       (sectNr) * SECTOR_SIZE,		\
                				       SECTOR_SIZE, /* write one sector */ \
                				       P_TASK_FS,				\
                				       fsbuf);

#define	NR_DEFAULT_FILE_SECTS	2048 /* 2048 * 512 = 1MB */

/* INODE::i_mode (octal, lower 12 bits reserved) */
#define I_TYPE_MASK     0170000
#define I_REGULAR       0100000
#define I_BLOCK_SPECIAL 0060000
#define I_DIRECTORY     0040000
#define I_CHAR_SPECIAL  0020000
#define I_NAMED_PIPE	0010000

#define	MAX_PATH	128



int doOpen();
int doClose();
struct SuperBlock* getSuperBlock(int dev);
int stripPath(char* filename, const char* pathname, struct Inode** ppinode);
int rwSector(int io_type, int dev, u64 pos, int bytes, int procNr, void* buf);
int searchFile(char* path);
struct Inode* getInode(int dev, int num);
void putInode(struct Inode* pinode);
void syncInode(struct Inode* p);
int doRdWt();
int doUnlink();



#endif /* _FS_H_ */
