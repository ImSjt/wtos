#include "proc.h"
#include "ipc.h"
#include "string.h"
#include "stdio.h"
#include "irq.h"
#include "hd.h"
#include "proto.h"

static u8 hdStatus;
static u8 hdbuf[SECTOR_SIZE * 2];

static void hdHandler(int irq);
static void hdCmdOut(struct HdCmd* cmd);
static int waitfor(int mask, int val, int timeout);
static void printIdentifyInfo(u16* hdinfo);
static void interruptWait();

static void initHd()
{
	u8 * nrDrives = (u8*)(0x475);
	printf("NrDrives:%d.\n", *nrDrives);
	assert(*nrDrives);

    putIrqHandler(AT_WINI_IRQ, hdHandler);
    enableIrq(CASCADE_IRQ); /* 主8259A */
    enableIrq(AT_WINI_IRQ); /* 从8259A */
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
		printf("%s: %s\n", iinfo[k].desc, s);
	}

	int capabilities = hdinfo[49];
	printf("LBA supported: %s\n",
	       (capabilities & 0x0200) ? "Yes" : "No");

	int cmdSetSupported = hdinfo[83];
	printf("LBA48 supported: %s\n",
	       (cmdSetSupported & 0x0400) ? "Yes" : "No");

	int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
	printf("HD size: %dMB\n", sectors * 512 / 1000000);

}
