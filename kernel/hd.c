#include "proc.h"
#include "ipc.h"
#include "string.h"
#include "stdio.h"
#include "irq.h"
#include "hd.h"
#include "proto.h"

static u8 hdStatus;
static u8 hdbuf[SECTOR_SIZE * 2];
static struct HdInfo hdinfo[1];

#define	DRV_OF_DEV(dev) (dev <= MAX_PRIM ? \
			 dev / NR_PRIM_PER_DRIVE : \
			 (dev - MINOR_hd1a) / NR_SUB_PER_DRIVE)

static void hdHandler(int irq);
static void hdCmdOut(struct HdCmd* cmd);
static int waitfor(int mask, int val, int timeout);
static void printIdentifyInfo(u16* hdinfo);
static void interruptWait();
static void hdOpen(int device);
static void partition(int device, int style);
static void printHdinfo(struct HdInfo* hdi);
static void getPartTable(int drive, int sectNr, struct PartEnt* entry);
static void hdClose(int device);
static void hdRdWt(struct Message* p);
static void hdIoctl(struct Message* p);

static void initHd()
{
    int i;
	u8 * nrDrives = (u8*)(0x475);
	printk("NrDrives:%d.\n", *nrDrives);
	assert(*nrDrives);

    putIrqHandler(AT_WINI_IRQ, hdHandler);
    enableIrq(CASCADE_IRQ); /* 主8259A */
    enableIrq(AT_WINI_IRQ); /* 从8259A */

	for (i = 0; i < (sizeof(hdinfo) / sizeof(hdinfo[0])); i++)
		memset(&hdinfo[i], 0, sizeof(hdinfo[0]));
	hdinfo[0].openCnt = 0;

}

static void hdIdentify(int drive)
{
    struct HdCmd cmd;
    cmd.device = MAKE_DEVICE_REG(0, drive, 0);
    cmd.command = ATA_IDENTIFY;
    hdCmdOut(&cmd);
    interruptWait();
    portRead(REG_DATA, hdbuf, SECTOR_SIZE);
    printIdentifyInfo((u16*)hdbuf);
}

/* 磁盘驱动程序 */
void taskHd()
{
    struct Message msg;

    /* 初始化 */
    initHd();

    while(1)
    {
        memset(&msg, 0, sizeof(msg));

        ipc(RECEIVE, ANY, &msg);

        int src = msg.source;

        switch(msg.type)
        {
        case DEV_OPEN:
            hdOpen(msg.DEVICE);
            break;

        case DEV_CLOSE:
            hdClose(msg.DEVICE);
            break;

        case DEV_READ:
        case DEV_WRITE:
            hdRdWt(&msg);
            break;

        case DEV_IOCTL:
            hdIoctl(&msg);
            break;

        default:
            spin("FS::main_loop (invalid msg.type)");
            break;
        }

        ipc(SEND, src, &msg);
    }
}

/* 中断处理，唤醒进程 */
static void hdHandler(int irq)
{
	hdStatus = inByte(REG_STATUS);

	informInt(P_TASK_HD);
}

static void hdCmdOut(struct HdCmd* cmd)
{
    /* 等待硬盘准备好 */
	if (!waitfor(STATUS_BSY, 0, HD_TIMEOUT))
		panic("hd error.");

	/* Activate the Interrupt Enable (nIEN) bit */
	outByte(REG_DEV_CTRL, 0);
	/* Load required parameters in the Command Block Registers */
	outByte(REG_FEATURES, cmd->features);
	outByte(REG_NSECTOR, cmd->count);
	outByte(REG_LBA_LOW, cmd->lbaLow);
	outByte(REG_LBA_MID, cmd->lbaMid);
	outByte(REG_LBA_HIGH, cmd->lbaHigh);
	outByte(REG_DEVICE, cmd->device);
	/* Write the command code to the Command Register */
	outByte(REG_CMD, cmd->command);
}

static int waitfor(int mask, int val, int timeout)
{
	int t = getTicks();

	while(((getTicks() - t) * 1000 / HZ) < timeout)
		if ((inByte(REG_STATUS) & mask) == val)
			return 1;

	return 0;
}

static void interruptWait()
{
	struct Message msg;
	ipc(RECEIVE, INTERRUPT, &msg);
}

static void printIdentifyInfo(u16* hdinfo)
{
	int i, k;
	char s[64];

	struct IdenInfoAscii
    {
		int idx;
		int len;
		char * desc;
	} iinfo[] = {
                    {10, 20, "HD SN"}, /* Serial number in ASCII */
		            {27, 40, "HD Model"} /* Model number in ASCII */
                };

	for (k = 0; k < sizeof(iinfo)/sizeof(iinfo[0]); k++)
    {
		char * p = (char*)&hdinfo[iinfo[k].idx];
		for (i = 0; i < iinfo[k].len/2; i++) {
			s[i*2+1] = *p++;
			s[i*2] = *p++;
		}
		s[i*2] = 0;
		printk("%s: %s\n", iinfo[k].desc, s);
	}

	int capabilities = hdinfo[49];
	printk("LBA supported: %s\n",
	       (capabilities & 0x0200) ? "Yes" : "No");

	int cmdSetSupported = hdinfo[83];
	printk("LBA48 supported: %s\n",
	       (cmdSetSupported & 0x0400) ? "Yes" : "No");

	int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
	printk("HD size: %dMB\n", sectors * 512 / 1000000);

}

static void hdOpen(int device)
{
    int drive = DRV_OF_DEV(device);
    assert(drive == 0);

    hdIdentify(drive);

    if(hdinfo[drive].openCnt++ == 0)
    {
        partition(drive * (NR_PART_PER_DRIVE + 1), P_PRIMARY);
        printHdinfo(&hdinfo[drive]);
    }
}

static void partition(int device, int style)
{
	int i;
	int drive = DRV_OF_DEV(device);
	struct HdInfo* hdi = &hdinfo[drive];

	struct PartEnt partTbl[NR_SUB_PER_DRIVE];

	if(style == P_PRIMARY)
    {
		getPartTable(drive, drive, partTbl);

		int nrPrimParts = 0;
		for (i = 0; i < NR_PART_PER_DRIVE; i++)
        {
			if (partTbl[i].sysId == NO_PART) 
				continue;

			nrPrimParts++;
			int devNr = i + 1;		  /* 1~4 */
			hdi->primary[devNr].base = partTbl[i].startSect;
			hdi->primary[devNr].size = partTbl[i].nrSects;

			if (partTbl[i].sysId == EXT_PART) /* extended */
				partition(device + devNr, P_EXTENDED);
		}
		assert(nrPrimParts != 0);
	}
	else if (style == P_EXTENDED)
    {
		int j = device % NR_PRIM_PER_DRIVE; /* 1~4 */
		int extStartSect = hdi->primary[j].base;
		int s = extStartSect;
		int nr1stSub = (j - 1) * NR_SUB_PER_PART; /* 0/16/32/48 */

		for (i = 0; i < NR_SUB_PER_PART; i++)
        {
			int dev_nr = nr1stSub + i;/* 0~15/16~31/32~47/48~63 */

			getPartTable(drive, s, partTbl);

			hdi->logical[dev_nr].base = s + partTbl[0].startSect;
			hdi->logical[dev_nr].size = partTbl[0].nrSects;

			s = extStartSect + partTbl[1].startSect;

			/* no more logical partitions
			   in this extended partition */
			if (partTbl[1].sysId == NO_PART)
				break;
		}
	}
	else
    {
		assert(0);
	}

}

static void getPartTable(int drive, int sectNr, struct PartEnt* entry)
{
	struct HdCmd cmd;
	cmd.features	= 0;
	cmd.count	= 1;
	cmd.lbaLow	= sectNr & 0xFF;
	cmd.lbaMid	= (sectNr >>  8) & 0xFF;
	cmd.lbaHigh	= (sectNr >> 16) & 0xFF;
	cmd.device	= MAKE_DEVICE_REG(1, /* LBA mode*/
					  drive,
					  (sectNr >> 24) & 0xF);
	cmd.command	= ATA_READ;
	hdCmdOut(&cmd);
	interruptWait();

	portRead(REG_DATA, hdbuf, SECTOR_SIZE);
	memcpy(entry,
	       hdbuf + PARTITION_TABLE_OFFSET,
	       sizeof(struct PartEnt) * NR_PART_PER_DRIVE);

}

static void printHdinfo(struct HdInfo* hdi)
{
	int i;
	for (i = 0; i < NR_PART_PER_DRIVE + 1; i++)
    {
		printk("%sPART_%d: base %d(0x%x), size %d(0x%x) (in sector)\n",
		       i == 0 ? " " : "     ",
		       i,
		       hdi->primary[i].base,
		       hdi->primary[i].base,
		       hdi->primary[i].size,
		       hdi->primary[i].size);
	}
    
	for (i = 0; i < NR_SUB_PER_DRIVE; i++) {
		if (hdi->logical[i].size == 0)
			continue;
		printk("         "
		       "%d: base %d(0x%x), size %d(0x%x) (in sector)\n",
		       i,
		       hdi->logical[i].base,
		       hdi->logical[i].base,
		       hdi->logical[i].size,
		       hdi->logical[i].size);
	}
}

static void hdClose(int device)
{
    int drive = DRV_OF_DEV(device);
    assert(drive == 0);

    hdinfo[drive].openCnt--;
}

static void hdRdWt(struct Message* p)
{
	int drive = DRV_OF_DEV(p->DEVICE);

	u64 pos = p->POSITION;
	assert((pos >> SECTOR_SIZE_SHIFT) < (1 << 31));

	/**
	 * We only allow to R/W from a SECTOR boundary:
	 */
	assert((pos & 0x1FF) == 0);

	u32 sectNr = (u32)(pos >> SECTOR_SIZE_SHIFT); /* pos / SECTOR_SIZE，扇区 */
	int logidx = (p->DEVICE - MINOR_hd1a) % NR_SUB_PER_DRIVE; /* 逻辑扇区号 */

    /* 定位到真正的扇区号 */
	sectNr += p->DEVICE < MAX_PRIM ?
		hdinfo[drive].primary[p->DEVICE].base :
		hdinfo[drive].logical[logidx].base;

    /* 读扇区 */
	struct HdCmd cmd;
	cmd.features	= 0;
	cmd.count	= (p->CNT + SECTOR_SIZE - 1) / SECTOR_SIZE;
	cmd.lbaLow	= sectNr & 0xFF;
	cmd.lbaMid	= (sectNr >>  8) & 0xFF;
	cmd.lbaHigh	= (sectNr >> 16) & 0xFF;
	cmd.device	= MAKE_DEVICE_REG(1, drive, (sectNr >> 24) & 0xF);
	cmd.command	= (p->type == DEV_READ) ? ATA_READ : ATA_WRITE;
	hdCmdOut(&cmd);

	int bytesLeft = p->CNT;
	void* la = p->BUF;

	while (bytesLeft)
    {
		int bytes = Min(SECTOR_SIZE, bytesLeft);
		if (p->type == DEV_READ)
        {
			interruptWait();
			portRead(REG_DATA, hdbuf, SECTOR_SIZE);
            memcpy(la, hdbuf, bytes);
		}
		else
        {
			if (!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT))
				panic("hd writing error.");

			portWrite(REG_DATA, la, bytes);
			interruptWait();
		}
        
		bytesLeft -= SECTOR_SIZE;
		la += SECTOR_SIZE;
	}
}

static void hdIoctl(struct Message* p)
{
	int device = p->DEVICE;
	int drive = DRV_OF_DEV(device);

	struct HdInfo* hdi = &hdinfo[drive];

	if (p->REQUEST == DIOCTL_GET_GEO)
    {
		void * dst = p->BUF;
		void * src = device < MAX_PRIM ? &hdi->primary[device] :
    				   &hdi->logical[(device - MINOR_hd1a) % NR_SUB_PER_DRIVE];

        memcpy(dst, src, sizeof(struct PartInfo));
	}
	else
    {
		assert(0);
	}
}
