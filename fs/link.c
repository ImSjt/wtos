#include "fs.h"
#include "string.h"
#include "ipc.h"
#include "stdio.h"
#include "global.h"

int doUnlink()
{
	char pathname[MAX_PATH];

	/* get parameters from the message */
	int nameLen = fsMsg.NAME_LEN;	/* length of filename */
	int src = fsMsg.source;	/* caller proc nr. */
	assert(nameLen < MAX_PATH);
    memcpy(pathname, fsMsg.PATHNAME, nameLen);
	pathname[nameLen] = 0;

	if (strcmp(pathname , "/") == 0)
    {
		printk("FS:do_unlink():: cannot unlink the root\n");
		return -1;
	}

	int inodeNr = searchFile(pathname);
	if (inodeNr == INVALID_INODE)
    {   
        /* file not found */
		printk("FS::do_unlink():: search_file() returns "
			"invalid inode: %s\n", pathname);
		return -1;
	}

	char filename[MAX_PATH];
	struct Inode* dirInode;
	if (stripPath(filename, pathname, &dirInode) != 0)
		return -1;

	struct Inode* pin = getInode(dirInode->iDev, inodeNr);

	if (pin->iMode != I_REGULAR)
    {
        /* can only remove regular files */
		printk("cannot remove file %s, because "
		       "it is not a regular file.\n",
		       pathname);
		return -1;
	}

	if (pin->iCnt > 1) {
        /* the file was opened */
		printk("cannot remove file %s, because pin->i_cnt is %d.\n",
		       pathname, pin->iCnt);
		return -1;
	}

	struct SuperBlock* sb = getSuperBlock(pin->iDev);

	/*************************/
	/* free the bit in i-map */
	/*************************/
	int byteIdx = inodeNr / 8;
	int bitIdx = inodeNr % 8;
	assert(byteIdx < SECTOR_SIZE);	/* we have only one i-map sector */
	/* read sector 2 (skip bootsect and superblk): */
	RD_SECT(pin->iDev, 2);
	assert(fsbuf[byteIdx % SECTOR_SIZE] & (1 << bitIdx));
	fsbuf[byteIdx % SECTOR_SIZE] &= ~(1 << bitIdx);
	WR_SECT(pin->iDev, 2);

	/**************************/
	/* free the bits in s-map */
	/**************************/
	bitIdx  = pin->iStartSect - sb->n1stSect + 1;
	byteIdx = bitIdx / 8;
	int bitsLeft = pin->iNrSects;
	int byteCnt = (bitsLeft - (8 - (bitIdx % 8))) / 8;

	/* current sector nr. */
	int s = 2  /* 2: bootsect + superblk */
		+ sb->nrImapSects + byteIdx / SECTOR_SIZE;

	RD_SECT(pin->iDev, s);

	int i;
	/* clear the first byte */
	for (i = bitIdx % 8; (i < 8) && bitsLeft; i++,bitsLeft--)
    {
		assert((fsbuf[byteIdx % SECTOR_SIZE] >> i & 1) == 1);
		fsbuf[byteIdx % SECTOR_SIZE] &= ~(1 << i);
	}

	/* clear bytes from the second byte to the second to last */
	int k;
	i = (byteIdx % SECTOR_SIZE) + 1;	/* the second byte */
	for (k = 0; k < byteCnt; k++,i++,bitsLeft-=8)
    {
		if (i == SECTOR_SIZE)
        {
			i = 0;
			WR_SECT(pin->iDev, s);
			RD_SECT(pin->iDev, ++s);
		}
		assert(fsbuf[i] == 0xFF);
		fsbuf[i] = 0;
	}

	/* clear the last byte */
	if (i == SECTOR_SIZE)
    {
		i = 0;
		WR_SECT(pin->iDev, s);
		RD_SECT(pin->iDev, ++s);
	}
	unsigned char mask = ~((unsigned char)(~0) << bitsLeft);
	assert((fsbuf[i] & mask) == mask);
	fsbuf[i] &= (~0) << bitsLeft;
	WR_SECT(pin->iDev, s);

	/***************************/
	/* clear the i-node itself */
	/***************************/
	pin->iMode = 0;
	pin->iSize = 0;
	pin->iStartSect = 0;
	pin->iNrSects = 0;
	syncInode(pin);
	/* release slot in inode_table[] */
	putInode(pin);

	/************************************************/
	/* set the inode-nr to 0 in the directory entry */
	/************************************************/
	int dirBlk0Nr = dirInode->iStartSect;
	int nrDirBlks = (dirInode->iSize + SECTOR_SIZE) / SECTOR_SIZE;
	int nrDirEntries =
		dirInode->iSize / DIR_ENTRY_SIZE; /* including unused slots
						     * (the file has been
						     * deleted but the slot
						     * is still there)
						     */
	int m = 0;
	struct DirEntry* pde = 0;
	int flg = 0;
	int dirSize = 0;

	for (i = 0; i < nrDirBlks; i++)
    {
		RD_SECT(dirInode->iDev, dirBlk0Nr + i);

		pde = (struct DirEntry *)fsbuf;
		int j;
		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++)
        {
			if (++m > nrDirEntries)
				break;

			if (pde->inodeNr == inodeNr)
            {
				/* pde->inode_nr = 0; */
				memset(pde, 0, DIR_ENTRY_SIZE);
				WR_SECT(dirInode->iDev, dirBlk0Nr + i);
				flg = 1;
				break;
			}

			if (pde->inodeNr != INVALID_INODE)
				dirSize += DIR_ENTRY_SIZE;
		}

		if (m > nrDirEntries || /* all entries have been iterated OR */
		    flg) /* file is found */
			break;
	}
	assert(flg);
	if (m == nrDirEntries)
    {
        /* the file is the last one in the dir */
		dirInode->iSize = dirSize;
		syncInode(dirInode);
	}

	return 0;
}

