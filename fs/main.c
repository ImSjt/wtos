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
static void readSuperBlock(int dev);

void taskFs()
{
	printk("Task FS begins.\n");

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
        case READ:
        case WRITE:
            fsMsg.CNT = doRdWt();
            break;
        /* case LSEEK: */
        /*  fs_msg.OFFSET = do_lseek(); */
        /*  break; */
        case UNLINK:
            fsMsg.RETVAL = doUnlink();
            break;
        case RESUME_PROC:
            src = fsMsg.PROC_NR;
            break;
        case SUSPEND_PROC:
            break;
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
		if (fsMsg.type != SUSPEND_PROC)
        {
            fsMsg.type = SYSCALL_RET;
            ipc(SEND, src, &fsMsg);
		}
    }
}

static void devOpen()
{
    /* 打开硬盘 */
	struct Message driverMsg;
	driverMsg.type = DEV_OPEN;
    driverMsg.DEVICE = MINOR(ROOT_DEV);
    assert(ddmap[MAJOR(ROOT_DEV)].driverNr != INVALID_DRIVER);    
	ipc(BOTH, P_TASK_HD, &driverMsg);
}

static void initFs()
{
    memset(fDescTable, 0, sizeof(fDescTable));
    memset(inodeTable, 0, sizeof(inodeTable));
    memset(superBlock, 0, sizeof(superBlock));

    struct SuperBlock* sb = superBlock;
    for(; sb < &superBlock[NR_SUPER_BLOCK]; sb++)
    {
        sb->sbDev = NO_DEV;
    }
    
    devOpen();

    /* 创建文件系统 */
    mkfs();

    /* 读取超级块到内存中 */
    readSuperBlock(ROOT_DEV);

    sb = getSuperBlock(ROOT_DEV);
    assert(sb->magic == MAGIC_V1);

    /* 读取根目录的inode到内存中 */
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

	printk("dev size: 0x%x sectors\n", geo.size);

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

	printk("devbase:0x%x00, sb:0x%x00, imap:0x%x00, smap:0x%x00\n"
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
int rwSector(int ioType, int dev, u64 pos, int bytes, int procNr, void* buf)
{
	struct Message driverMsg;

	driverMsg.type		= ioType;
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
	struct SuperBlock * sb = getSuperBlock(p->iDev);
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

int doRdWt()
{
	int fd = fsMsg.FD;	/**< file descriptor. */
	void * buf = fsMsg.BUF;/**< r/w buffer */
	int len = fsMsg.CNT;	/**< r/w bytes */

	int src = fsMsg.source;		/* caller proc nr. */

	assert((pcaller->filp[fd] >= &fDescTable[0]) &&
	       (pcaller->filp[fd] < &fDescTable[NR_FILE_DESC]));

	if (!(pcaller->filp[fd]->fdMode & O_RDWR))
		return -1;

    /* 读写位置 */
	int pos = pcaller->filp[fd]->fdPos;

    /* 文件对应的inode */
	struct Inode* pin = pcaller->filp[fd]->fdInode;

	assert(pin >= &inodeTable[0] && pin < &inodeTable[NR_INODE]);

	int imode = pin->iMode & I_TYPE_MASK;

    /* 字符设备 */
	if (imode == I_CHAR_SPECIAL)
    {
		int t = fsMsg.type == READ ? DEV_READ : DEV_WRITE;
		fsMsg.type = t;

        /* 文件起始的扇区 */
		int dev = pin->iStartSect;
		assert(MAJOR(dev) == 4);

        /* 将消息发送给驱动程序 */
		fsMsg.DEVICE	= MINOR(dev);
		fsMsg.BUF	    = buf;
		fsMsg.CNT	    = len;
		fsMsg.PROC_NR	= src;
		assert(ddmap[MAJOR(dev)].driverNr != INVALID_DRIVER);
		ipc(BOTH, ddmap[MAJOR(dev)].driverNr, &fsMsg);
		assert(fsMsg.CNT == len);

		return fsMsg.CNT;
	}
	else
    {
		assert(pin->iMode == I_REGULAR || pin->iMode == I_DIRECTORY);
		assert((fsMsg.type == READ) || (fsMsg.type == WRITE));

        /* 允许读到的最后位置 */
		int posEnd;
		if (fsMsg.type == READ)
			posEnd = Min(pos + len, pin->iSize);
		else		/* WRITE */
			posEnd = Min(pos + len, pin->iNrSects * SECTOR_SIZE);

		int off = pos % SECTOR_SIZE;
		int rwSectMin=pin->iStartSect+(pos>>SECTOR_SIZE_SHIFT);     /* 起始扇区 */
		int rwSectMax=pin->iStartSect+(posEnd>>SECTOR_SIZE_SHIFT);  /* 结束扇区 */

		int chunk = Min(rwSectMax - rwSectMin + 1,
				FSBUF_SIZE >> SECTOR_SIZE_SHIFT);

		int bytesRw = 0;
		int bytesLeft = len;
		int i;
		for (i = rwSectMin; i <= rwSectMax; i += chunk)
        {
			/* read/write this amount of bytes every time */
			int bytes = Min(bytesLeft, chunk * SECTOR_SIZE - off);

            /* 读扇区 */
			rwSector(DEV_READ,
				  pin->iDev,
				  i * SECTOR_SIZE,
				  chunk * SECTOR_SIZE,
				  P_TASK_FS,
				  fsbuf);

			if (fsMsg.type == READ)
            {
                memcpy((void*)(buf+bytesRw), (void*)(fsbuf+off), bytes);
			}
			else
            {   /* WRITE */
                memcpy((void*)(fsbuf+off), (void*)(buf+bytesRw), bytes);
				rwSector(DEV_WRITE,
					  pin->iDev,
					  i * SECTOR_SIZE,
					  chunk * SECTOR_SIZE,
					  P_TASK_FS,
					  fsbuf);
			}
			off = 0;
			bytesRw += bytes;
			pcaller->filp[fd]->fdPos += bytes; /* 移动文件指针 */
			bytesLeft -= bytes;
		}

		if (pcaller->filp[fd]->fdPos > pin->iSize)
        {
			/* update inode::size */
			pin->iSize = pcaller->filp[fd]->fdPos;

			/* write the updated i-node back to disk */
			syncInode(pin); /* 更新文件系统中的inode */
		}

		return bytesRw;
	}
}
