#include "fs.h"
#include "global.h"
#include "string.h"
#include "hd.h"

int searchFile(char* path)
{
	int i, j;

	char filename[MAX_PATH];
	memset(filename, 0, MAX_FILENAME_LEN);
	struct Inode* dirInode;

    /* 解析路径：目录+文件 */
	if (stripPath(filename, path, &dirInode) != 0)
		return 0;

	if (filename[0] == 0)	/* path: "/" */
		return dirInode->iNum;

	/**
	 * Search the dir for the file.
	 */
	int dirBlk0Nr = dirInode->iStartSect;
	int nrDirBlks = (dirInode->iSize + SECTOR_SIZE - 1) / SECTOR_SIZE;
	int nrDirEntries =
	  dirInode->iSize / DIR_ENTRY_SIZE; /**
					       * including unused slots
					       * (the file has been deleted
					       * but the slot is still there)
					       */
	int m = 0;
	struct DirEntry* pde;
	for (i = 0; i < nrDirBlks; i++)
    {
        /* 读取目录文件 */
		RD_SECT(dirInode->iDev, dirBlk0Nr + i);
		pde = (struct DirEntry *)fsbuf;
		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++)
        {
			if (memcmp(filename, pde->name, MAX_FILENAME_LEN) == 0)
				return pde->inodeNr;
			if (++m > nrDirEntries)
				break;
		}
		if (m > nrDirEntries) /* all entries have been iterated */
			break;
	}

	/* file not found */
	return 0;

}

int stripPath(char* filename, const char* pathname, struct Inode** ppinode)
{
	const char* s = pathname;
	char* t = filename;

	if (s == 0)
		return -1;

	if (*s == '/')
		s++;

	while (*s)
    {   
        /* check each character */
		if (*s == '/')
			return -1;
		*t++ = *s++;
		/* if filename is too long, just truncate it */
		if (t - filename >= MAX_FILENAME_LEN)
			break;
	}
	*t = 0;

	*ppinode = rootInode;

	return 0;
}

