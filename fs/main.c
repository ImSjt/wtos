#include "type.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
#include "stdio.h"
#include "hd.h"
#include "ipc.h"

static void initFs();
static void mkfs();
static int rwSector(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf);



void taskFs()
{
	printf("Task FS begins.\n");

    initFs();

	spin("FS");
}

static void initFs()
{
    /* 打开硬盘 */
	struct Message driverMsg;
	driverMsg.type = DEV_OPEN;
    driverMsg.DEVICE = MINOR(ROOT_DEV);
    assert(ddmap[MAJOR(ROOT_DEV)].driverNr != INVALID_DRIVER);    
	ipc(BOTH, P_TASK_HD, &driverMsg);

    /* 创建文件系统 */
    mkfs();
}

static void mkfs()
{
	struct Message driverMsg;
	int i, j;

	int bitsPerSect = SECTOR_SIZE * 8; /* 8 bits per byte */

	/* get the geometry of ROOTDEV */
	struct PartInfo geo;
	driverMsg.type		= DEV_IOCTL;
	driverMsg.DEVICE	= MINOR(ROOT_DEV);
	driverMsg.REQUEST	= DIOCTL_GET_GEO;
	driverMsg.BUF		= &geo;
	driverMsg.PROC_NR	= P_TASK_FS;
	assert(ddmap[MAJOR(ROOT_DEV)].driverNr != INVALID_DRIVER);
	ipc(BOTH, ddmap[MAJOR(ROOT_DEV)].driverNr, &driverMsg);

	printf("dev size: 0x%x sectors\n", geo.size);

	/************************/
	/*      super block     */
	/************************/
	struct SuperBlock sb;
	sb.magic	        = MAGIC_V1;
	sb.nrInodes	        = bitsPerSect; /* inode最大数量 */
	sb.nrInodeSects     = sb.nrInodes * INODE_SIZE / SECTOR_SIZE; /* inode占据多少个扇区 */
	sb.nrSects	        = geo.size; /* 扇区数 */
	sb.nrImapSects      = 1;
	sb.nrSmapSects      = sb.nrSects / bitsPerSect + 1;
	sb.n1stSect	        = 1 + 1 +   /* 起始扇区 */
                		sb.nrImapSects + sb.nrSmapSects + sb.nrInodeSects;
	sb.rootInode	    = ROOT_INODE; /* 根目录文件的inode标号 */
	sb.inodeSize	    = INODE_SIZE; /* 一个inode的大小 */

	struct Inode x;
	sb.inodeISizeOff    = (int)&x.iSize - (int)&x;
	sb.inodeStartOff    = (int)&x.iStartSect - (int)&x;
	sb.dirEntSize	    = DIR_ENTRY_SIZE;   /* 目录项的大小 */

	struct DirEntry de;
	sb.dirEntInodeOff = (int)&de.inodeNr - (int)&de;
	sb.dirEntFnameOff = (int)&de.name - (int)&de;

    /* fsbuf为缓冲区 */
	memset(fsbuf, 0x90, SECTOR_SIZE);
	memcpy(fsbuf, &sb, SUPER_BLOCK_SIZE); /* 写入超级块 */

	/* 写超级块 */
	WR_SECT(ROOT_DEV, 1);

	printf("devbase:0x%x00, sb:0x%x00, imap:0x%x00, smap:0x%x00\n"
	       "        inodes:0x%x00, 1st_sector:0x%x00\n", 
	       geo.base * 2,
	       (geo.base + 1) * 2,
	       (geo.base + 1 + 1) * 2,
	       (geo.base + 1 + 1 + sb.nrImapSects) * 2,
	       (geo.base + 1 + 1 + sb.nrImapSects + sb.nrSmapSects) * 2,
	       (geo.base + sb.n1stSect) * 2);

	/************************/
	/*       inode map      */
	/************************/
	memset(fsbuf, 0, SECTOR_SIZE);
	for (i = 0; i < (NR_CONSOLES + 2); i++)
		fsbuf[0] |= 1 << i;

	assert(fsbuf[0] == 0x1F);/* 0001 1111 : 
				  *    | ||||
				  *    | |||`--- bit 0 : reserved
				  *    | ||`---- bit 1 : the first inode,
				  *    | ||              which indicates `/'
				  *    | |`----- bit 2 : /dev_tty0
				  *    | `------ bit 3 : /dev_tty1
				  *    `-------- bit 4 : /dev_tty2
				  */
	WR_SECT(ROOT_DEV, 2);

	/************************/
	/*      secter map      */
	/************************/
	memset(fsbuf, 0, SECTOR_SIZE);
	int nrSects = NR_DEFAULT_FILE_SECTS + 1; /* 根目录文件对应的扇区数 */
	/*             ~~~~~~~~~~~~~~~~~~~|~   |
	 *                                |    `--- bit 0 is reserved
	 *                                `-------- for `/'
	 */
	for (i = 0; i < nrSects / 8; i++)
		fsbuf[i] = 0xFF;

	for (j = 0; j < nrSects % 8; j++)
		fsbuf[i] |= (1 << j);

	WR_SECT(ROOT_DEV, 2 + sb.nrImapSects);

	/* zeromemory the rest sector-map */
	memset(fsbuf, 0, SECTOR_SIZE);
	for (i = 1; i < sb.nrSmapSects; i++)
		WR_SECT(ROOT_DEV, 2 + sb.nrImapSects + i);

	/************************/
	/*       inodes         */
	/************************/
	/* inode of `/' */
	memset(fsbuf, 0, SECTOR_SIZE);
	struct Inode* pi = (struct Inode*)fsbuf;
	pi->iMode = I_DIRECTORY;    /* inode对应的文件模式 */
	pi->iSize = DIR_ENTRY_SIZE * 4; /* 4 files:
					  * `.',
					  * `dev_tty0', `dev_tty1', `dev_tty2',
					  */
	pi->iStartSect = sb.n1stSect;
	pi->iNrSects = NR_DEFAULT_FILE_SECTS;
    
	/* inode of `/dev_tty0~2' */
	for (i = 0; i < NR_CONSOLES; i++)
    {
		pi = (struct Inode*)(fsbuf + (INODE_SIZE * (i + 1)));
		pi->iMode = I_CHAR_SPECIAL;
		pi->iSize = 0;
		pi->iStartSect = MAKE_DEV(DEV_CHAR_TTY, i);
		pi->iNrSects = 0;
	}
	WR_SECT(ROOT_DEV, 2 + sb.nrImapSects + sb.nrSmapSects);

	/************************/
	/*          `/'         */
	/************************/
	memset(fsbuf, 0, SECTOR_SIZE);
	struct DirEntry* pde = (struct DirEntry *)fsbuf;

	pde->inodeNr = 1;
	strcpy(pde->name, ".");

	/* dir entries of `/dev_tty0~2' */
	for (i = 0; i < NR_CONSOLES; i++)
    {
		pde++;
		pde->inodeNr = i + 2; /* dev_tty0's inode_nr is 2 */
		sprintf(pde->name, "dev_tty%d", i);
	}

    /* 写入根文件目录 */
	WR_SECT(ROOT_DEV, sb.n1stSect);

}

/* 读写扇区 */
static int rwSector(int io_type, int dev, u64 pos, int bytes, int procNr, void* buf)
{
	struct Message driverMsg;

	driverMsg.type		= io_type;
	driverMsg.DEVICE	= MINOR(dev);
	driverMsg.POSITION	= pos;
	driverMsg.BUF		= buf;
	driverMsg.CNT		= bytes;
	driverMsg.PROC_NR	= procNr;
	assert(ddmap[MAJOR(dev)].driverNr != INVALID_DRIVER);
	ipc(BOTH, ddmap[MAJOR(dev)].driverNr, &driverMsg);

	return 0;
}

