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
static void readSuperBlock(int dev);

void taskFs()
{
	printf("Task FS begins.\n");

    initFs();

    while(1)
    {
        ipc(RECEIVE, ANY, &fsMsg);

        int src = fsMsg.source;
        pcaller = &procTable[src];

        switch (fsMsg.type)
        {
        case OPEN:
            fsMsg.FD = doOpen();
            break;

        case CLOSE:
            fsMsg.RETVAL = doClose();
            break;
        /* case READ: */
        /* case WRITE: */
        /*  fs_msg.CNT = do_rdwt(); */
        /*  break; */
        /* case LSEEK: */
        /*  fs_msg.OFFSET = do_lseek(); */
        /*  break; */
        /* case UNLINK: */
        /*  fs_msg.RETVAL = do_unlink(); */
        /*  break; */
        /* case RESUME_PROC: */
        /*  src = fs_msg.PROC_NR; */
        /*  break; */
        /* case FORK: */
        /*  fs_msg.RETVAL = fs_fork(); */
        /*  break; */
        /* case EXIT: */
        /*  fs_msg.RETVAL = fs_exit(); */
        /*  break; */
        /* case STAT: */
        /*  fs_msg.RETVAL = do_stat(); */
        /*  break; */
        default:
            
            break;
        }

        fsMsg.type = SYSCALL_RET;
        ipc(SEND, src, &fsMsg);
    }
}

static void initFs()
{
    memset(fDescTable, 0, sizeof(fDescTable));
    memset(inodeTable, 0, sizeof(inodeTable));
    memset(superBlock, 0, sizeof(superBlock));

    struct SuperBlock* sb = superBlock;
    for(; sb < superBlock[NR_SUPER_BLOCK]; sb++)
    {
        sb->sbDev = NO_DEV;
    }
    
    /* 打开硬盘 */
	struct Message driverMsg;
	driverMsg.type = DEV_OPEN;
    driverMsg.DEVICE = MINOR(ROOT_DEV);
    assert(ddmap[MAJOR(ROOT_DEV)].driverNr != INVALID_DRIVER);    
	ipc(BOTH, P_TASK_HD, &driverMsg);

    /* 创建文件系统 */
    mkfs();

    readSuperBlock(ROOT_DEV);

    sb = getSuperBlock(ROOT_DEV);
    assert(sb->magic == MAGIC_V1);

    rootInode = getInode(ROOT_DEV, ROOT_INODE);
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

/* 从缓存区中查找inode，如果没有，则从文件系统中读取 */
struct Inode* getInode(int dev, int num)
{
	if (num == 0)
		return 0;

	struct Inode * p;
	struct Inode * q = 0;
	for (p = &inodeTable[0]; p < &inodeTable[NR_INODE]; p++)
    {
		if (p->iCnt)
        {
            /* not a free slot */
			if ((p->iDev == dev) && (p->iNum == num))
            {
				/* this is the inode we want */
				p->iCnt++;
				return p;
			}
		}
		else
        {
            /* a free slot */
			if (!q) /* q hasn't been assigned yet */
				q = p; /* q <- the 1st free slot */
		}
	}

	if (!q)
		panic("the inode table is full");

	q->iDev = dev;
	q->iNum = num;
	q->iCnt = 1;

	struct SuperBlock* sb = getSuperBlock(dev);
	int blkNr = 1 + 1 + sb->nrImapSects + sb->nrSmapSects +
		((num - 1) / (SECTOR_SIZE / INODE_SIZE));
	RD_SECT(dev, blkNr);
	struct Inode* pinode =
		(struct Inode*)((u8*)fsbuf +
				((num - 1 ) % (SECTOR_SIZE / INODE_SIZE))
				 * INODE_SIZE);
	q->iMode = pinode->iMode;
	q->iSize = pinode->iSize;
	q->iStartSect = pinode->iStartSect;
	q->iNrSects = pinode->iNrSects;
	return q;

}

void putInode(struct Inode* pinode)
{
	assert(pinode->iCnt > 0);
	pinode->iCnt--;
}

/* 同步内存与文件系统中的inode */
void syncInode(struct Inode* p)
{
	struct Inode* pinode;
	struct SuperBlock * sb = getSuperBlock(p->i_dev);
	int blk_nr = 1 + 1 + sb->nrImapSects + sb->nrSmapSects +
		((p->iNum - 1) / (SECTOR_SIZE / INODE_SIZE));
	RD_SECT(p->iDev, blk_nr);
	pinode = (struct Inode*)((u8*)fsbuf +
				 (((p->iNum - 1) % (SECTOR_SIZE / INODE_SIZE))
				  * INODE_SIZE));
	pinode->iMode = p->iMode;
	pinode->iSize = p->iSize;
	pinode->iStartSect = p->iStartSect;
	pinode->iNrSects = p->iNrSects;
	WR_SECT(p->iDev, blk_nr);
}

struct SuperBlock* getSuperBlock(int dev)
{
	struct SuperBlock* sb = superBlock;
	for (; sb < &superBlock[NR_SUPER_BLOCK]; sb++)
		if (sb->sbDev == dev)
			return sb;

	panic("super block of devie %d not found.\n", dev);

	return 0;
}

static void readSuperBlock(int dev)
{
	int i;
	struct Message driverMsg;

	driverMsg.type		= DEV_READ;
	driverMsg.DEVICE	= MINOR(dev);
	driverMsg.POSITION	= SECTOR_SIZE * 1;
	driverMsg.BUF		= fsbuf;
	driverMsg.CNT		= SECTOR_SIZE;
	driverMsg.PROC_NR	= P_TASK_FS;
	assert(ddmap[MAJOR(dev)].driverNr != INVALID_DRIVER);
	ipc(BOTH, ddmap[MAJOR(dev)].driverNr, &driverMsg);

	/* find a free slot in super_block[] */
	for (i = 0; i < NR_SUPER_BLOCK; i++)
		if (superBlock[i].sbDev == NO_DEV)
			break;
	if (i == NR_SUPER_BLOCK)
		panic("super_block slots used up");

	assert(i == 0); /* currently we use only the 1st slot */

	struct SuperBlock * psb = (struct SuperBlock *)fsbuf;

	superBlock[i] = *psb;
	superBlock[i].sbDev = dev;
}

