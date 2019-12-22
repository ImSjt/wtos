#include "fs.h"
#include "global.h"
#include "stdio.h"
#include "ipc.h"
#include "string.h"

static struct Inode* createFile(char* path, int flags);
static int allocSmapBit(int dev, int nrSectsToAlloc);
static struct Inode* newInode(int dev, int inodeNr, int startSect);
static int allocImapBit(int dev);
static void newDirEntry(struct Inode* dirInode,int inodeNr,char* filename);

int doOpen()
{
	int fd = -1;		/* return value */

	char pathname[MAX_PATH];

	/* get parameters from the message */
	int flags = fsMsg.FLAGS;	/* access mode */
	int nameLen = fsMsg.NAME_LEN;	/* length of filename */
	int src = fsMsg.source;	/* caller proc nr. */
	assert(nameLen < MAX_PATH);
    memcpy(pathname, fsMsg.PATHNAME, nameLen);
	pathname[nameLen] = 0;

    /* 在进程的文件描述表里面找到第一个空闲的项 */
	int i;
	for (i = 0; i < NR_FILES; i++)
    {
		if (pcaller->filp[i] == 0) {
			fd = i;
			break;
		}
	}
	if ((fd < 0) || (fd >= NR_FILES))
		panic("filp[] is full (PID:%d)", proc2pid(pcaller));

	/* 找到一个空闲的文件描述符 */
	for (i = 0; i < NR_FILE_DESC; i++)
		if (fDescTable[i].fdInode == 0)
			break;

	if (i >= NR_FILE_DESC)
		panic("fDescTable[] is full (PID:%d)", proc2pid(pcaller));

    /* 查找文件是否存在文件系统中 */
	int inodeNr = searchFile(pathname);

	struct Inode* pin = 0;
	if (flags & O_CREAT)
    {
		if (inodeNr)
        {
			printk("file exists.\n");
			return -1;
		}
		else
        {
			pin = createFile(pathname, flags);
		}
	}
	else
    {
		assert(flags & O_RDWR);

		char filename[MAX_PATH];
		struct Inode* dirInode;
		if (stripPath(filename, pathname, &dirInode) != 0)
			return -1;
		pin = getInode(dirInode->iDev, inodeNr);
	}

	if (pin)
    {
		/* connects proc with file_descriptor */
		pcaller->filp[fd] = &fDescTable[i];

		/* connects file_descriptor with inode */
		fDescTable[i].fdInode = pin;

		fDescTable[i].fdMode = flags;
		/* f_desc_table[i].fd_cnt = 1; */
		fDescTable[i].fdPos = 0;

		int imode = pin->iMode & I_TYPE_MASK;

		if (imode == I_CHAR_SPECIAL)
        {
			struct Message driverMsg;

			driverMsg.type = DEV_OPEN;
			int dev = pin->iStartSect;
			driverMsg.DEVICE = MINOR(dev);
			assert(MAJOR(dev) == 4);
			assert(ddmap[MAJOR(dev)].driverNr != INVALID_DRIVER);

			ipc(BOTH, ddmap[MAJOR(dev)].driverNr,  &driverMsg);
		}
		else if (imode == I_DIRECTORY)
        {
			assert(pin->iNum == ROOT_INODE);
		}
		else
        {
			assert(pin->iMode == I_REGULAR);
		}
	}
	else
    {
		return -1;
	}

	return fd;
}

int doClose()
{
	int fd = fsMsg.FD;
	putInode(pcaller->filp[fd]->fdInode);
	pcaller->filp[fd]->fdInode = 0;
	pcaller->filp[fd] = 0;

	return 0;
}

static struct Inode* createFile(char* path, int flags)
{
	char filename[MAX_PATH];
	struct Inode* dirInode;

	if (stripPath(filename, path, &dirInode) != 0)
		return 0;

    /* 设置inode map */
	int inodeNr = allocImapBit(dirInode->iDev);

    /* 设置sector map */
	int freeSectNr = allocSmapBit(dirInode->iDev, NR_DEFAULT_FILE_SECTS);

    /* 新分配一个inode */
	struct Inode* newino = newInode(dirInode->iDev, inodeNr, freeSectNr);

    /* 在文件中创建新的文件项 */
	newDirEntry(dirInode, newino->iNum, filename);

	return newino;
}

/* 在文件系统中分配一个inode */
static int allocImapBit(int dev)
{
	int inodeNr = 0;
	int i, j, k;

	int imapBlk0Nr = 1 + 1; /* 1 boot sector & 1 super block */
	struct SuperBlock* sb = getSuperBlock(dev);

	for (i = 0; i < sb->nrImapSects; i++)
    {
		RD_SECT(dev, imapBlk0Nr + i);

		for (j = 0; j < SECTOR_SIZE; j++)
        {
			/* skip `11111111' bytes */
			if (fsbuf[j] == 0xFF)
				continue;

			/* skip `1' bits */
			for (k = 0; ((fsbuf[j] >> k) & 1) != 0; k++) {}

			/* i: sector index; j: byte index; k: bit index */
			inodeNr = (i * SECTOR_SIZE + j) * 8 + k;
			fsbuf[j] |= (1 << k);

			/* write the bit to imap */
			WR_SECT(dev, imapBlk0Nr + i);
			break;
		}

		return inodeNr;
	}

	/* no free bit in imap */
	panic("inode-map is probably full.\n");

	return 0;
}

static int allocSmapBit(int dev, int nrSectsToAlloc)
{
	/* int nr_sects_to_alloc = NR_DEFAULT_FILE_SECTS; */

	int i; /* sector index */
	int j; /* byte index */
	int k; /* bit index */

	struct SuperBlock* sb = getSuperBlock(dev);

	int smapBlk0Nr = 1 + 1 + sb->nrImapSects;
	int freeSectNr = 0;

	for (i = 0; i < sb->nrSmapSects; i++)
    {
        /* smap_blk0_nr + i :
         * current sect nr.
         */
		RD_SECT(dev, smapBlk0Nr + i);

		/* byte offset in current sect */
		for (j = 0; j < SECTOR_SIZE && nrSectsToAlloc > 0; j++)
        {
			k = 0;
			if (!freeSectNr)
            {
				/* loop until a free bit is found */
				if (fsbuf[j] == 0xFF) continue;
				for (; ((fsbuf[j] >> k) & 1) != 0; k++) {}
				freeSectNr = (i * SECTOR_SIZE + j) * 8 +
					k - 1 + sb->n1stSect;
			}

			for (; k < 8; k++)
            {
                /* repeat till enough bits are set */
				assert(((fsbuf[j] >> k) & 1) == 0);
				fsbuf[j] |= (1 << k);
				if (--nrSectsToAlloc == 0)
					break;
			}
		}

		if (freeSectNr) /* free bit found, write the bits to smap */
			WR_SECT(dev, smapBlk0Nr + i);

		if (nrSectsToAlloc == 0)
			break;
	}

	assert(nrSectsToAlloc == 0);

	return freeSectNr;
}

static struct Inode * newInode(int dev, int inodeNr, int startSect)
{
	struct Inode* newInode = getInode(dev, inodeNr);

	newInode->iMode = I_REGULAR;
	newInode->iSize = 0;
	newInode->iStartSect = startSect;
	newInode->iNrSects = NR_DEFAULT_FILE_SECTS;

	newInode->iDev = dev;
	newInode->iCnt = 1;
	newInode->iNum = inodeNr;

	/* write to the inode array */
	syncInode(newInode);

	return newInode;
}

static void newDirEntry(struct Inode* dirInode,int inodeNr,char* filename)
{
	/* write the dir_entry */
	int dirBlk0Nr = dirInode->iStartSect;
	int nrDirBlks = (dirInode->iSize + SECTOR_SIZE) / SECTOR_SIZE;
	int nrDirEntries =
		dirInode->iSize / DIR_ENTRY_SIZE; /**
						     * including unused slots
						     * (the file has been
						     * deleted but the slot
						     * is still there)
						     */
	int m = 0;
	struct DirEntry* pde;
	struct DirEntry* new_de = 0;

	int i, j;
	for (i = 0; i < nrDirBlks; i++)
    {
		RD_SECT(dirInode->iDev, dirBlk0Nr + i);

		pde = (struct DirEntry *)fsbuf;
		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++)
        {
			if (++m > nrDirEntries)
				break;

			if (pde->inodeNr == 0)
            {
                /* it's a free slot */
				new_de = pde;
				break;
			}
		}
        
		if (m > nrDirEntries ||/* all entries have been iterated or */
		    new_de)              /* free slot is found */
			break;
	}
	if (!new_de)
    { /* reached the end of the dir */
		new_de = pde;
		dirInode->iSize += DIR_ENTRY_SIZE;
	}
	new_de->inodeNr = inodeNr;
	strcpy(new_de->name, filename);

	/* write dir block -- ROOT dir block */
	WR_SECT(dirInode->iDev, dirBlk0Nr + i);

	/* update dir inode */
	syncInode(dirInode);
}

