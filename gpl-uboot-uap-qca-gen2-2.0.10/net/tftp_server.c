/*
 *	Copyright 1994, 1995, 2000 Neil Russell.
 *	(See License)
 *	Copyright 2000, 2001 DENX Software Engineering, Wolfgang Denk, wd@denx.de
 */

#include <common.h>
#include <command.h>
#include <net.h>
#include "tftp.h"
#include "bootp.h"
#include <jffs2/load_kernel.h>

#if 0
#undef	ET_DEBUG
#endif

#if (CONFIG_COMMANDS & CFG_CMD_TFTP_SERVER)

#define WELL_KNOWN_PORT	69		/* Well known TFTP port #		*/
#define TIMEOUT		1		/* Seconds to timeout for a lost pkt	*/
#ifndef	CONFIG_NET_RETRY_COUNT
# define TIMEOUT_COUNT	5		/* # of timeouts before giving up  */
#else
# define TIMEOUT_COUNT  (CONFIG_NET_RETRY_COUNT * 8)
#endif
					/* (for checking the image size)	*/
#define HASHES_PER_LINE	65		/* Number of "loading" hashes per line	*/

#define STR_IP "%d.%d.%d.%d"
#define IP_STR(a) \
	((unsigned char*)a)[0], ((unsigned char*)a)[1], \
	((unsigned char*)a)[2], ((unsigned char*)a)[3]

/*
 *	TFTP operations.
 */
#define TFTP_RRQ	1
#define TFTP_WRQ	2
#define TFTP_DATA	3
#define TFTP_ACK	4
#define TFTP_ERROR	5
#define TFTP_OACK	6

struct	tftphdr {
	short	opcode;			/* packet type */
	union {
		unsigned short	block;	/* block # */
		short	code;		/* error code */
		char	stuff[1];	/* request packet stuff */
	} __attribute__ ((__packed__)) th_u;
	char	data[1];		/* data or error string */
} __attribute__ ((__packed__));

static int	TftpClientPort;		/* The UDP port at their end		*/
static IPaddr_t	TftpOurClientIP;	/* Their IP address			*/
static int	TftpOurPort;		/* The UDP port at our end		*/
static int	TftpTimeoutCount;
static ulong	TftpBlock;		/* packet sequence number		*/
static ulong	TftpLastBlock;		/* last packet sequence number received */
static ulong	TftpBlockWrap;		/* count of sequence number wraparounds */
static ulong	TftpBlockWrapOffset;	/* memory offset due to wrapping	*/
static int	TftpServerState;
static ulong	TftpLastTimer;
static ulong	TftpFileAddr;		/* data address to upload when user issues "GET" */
static ulong	TftpFileSize;		/* data amount to upload to user	*/

//static int	ResetCfgToDefaults = 0;	/* Reset FW configuration (cfg partition) */
int	TftpServerOverwriteBootloader = 0; /* 1 - update U-Boot if found in FW */
int	TftpServerUseDefinedIP = 0; /* 1 - use default IP for server */

#define TFTP_SRV_STATE_CONNECT	1
#define TFTP_SRV_STATE_WRQ	2
#define TFTP_SRV_STATE_RRQ	3
#define TFTP_SRV_STATE_DATA	4
#define TFTP_SRV_STATE_TOO_LARGE	5
#define TFTP_SRV_STATE_BAD_MAGIC	6
#define TFTP_SRV_STATE_OACK	7
#define TFTP_SRV_STATE_BADFW	8
#define TFTP_SRV_STATE_FWUPDATE	9
#define TFTP_SRV_STATE_DATA_SEND	10

#define TFTP_BLOCK_SIZE		512		    /* default TFTP block size	*/
#define TFTP_SEQUENCE_SIZE	((ulong)(1<<16))    /* sequence number is 16 bit */

#define DEFAULT_NAME_LEN	32
static char tftp_filename[DEFAULT_NAME_LEN];

#ifdef CFG_DIRECT_FLASH_TFTP
extern flash_info_t flash_info[];
#endif

static u32  led_tick = 0;
extern void unifi_set_led(int);

static __inline__ void
store_block (unsigned block, uchar * src, unsigned len)
{
	ulong offset = block * TFTP_BLOCK_SIZE + TftpBlockWrapOffset;
	ulong newsize = offset + len;
#ifdef CFG_DIRECT_FLASH_TFTP
	int i, rc = 0;

	for (i=0; i<CFG_MAX_FLASH_BANKS; i++) {
		/* start address in flash? */
		if (load_addr + offset >= flash_info[i].start[0]) {
			rc = 1;
			break;
		}
	}

	if (rc) { /* Flash is destination for this packet */
		rc = flash_write ((char *)src, (ulong)(load_addr+offset), len);
		if (rc) {
			flash_perror (rc);
			NetState = NETLOOP_FAIL;
			return;
		}
	}
	else
#endif /* CFG_DIRECT_FLASH_TFTP */
	{
		(void)memcpy((void *)(load_addr + offset), src, len);
	}

	if (NetBootFileXferSize < newsize)
		NetBootFileXferSize = newsize;
}

/*
 *  Ubiquiti firmware stuff
 */

typedef struct header {
        char magic[4];
        char version[256];
        unsigned long crc;
        unsigned long pad;
} __attribute__ ((packed)) header_t;

typedef struct part {
        char magic[4];
        char name[16];
	char pad[12];
	unsigned long mem_addr;
        unsigned long id;
        unsigned long base_addr;
        unsigned long entry_addr;
        unsigned long length;
        unsigned long part_size;
} __attribute__ ((packed)) part_t;

typedef struct part_crc {
        unsigned long crc;
        unsigned long pad;
} __attribute__ ((packed)) part_crc_t;

typedef struct signature {
        char magic[4];
        unsigned long crc;
        unsigned long pad;
} __attribute__ ((packed)) signature_t;


typedef struct partition_data {
	part_t* header;
	unsigned char* data;
	part_crc_t* signature;
	unsigned int valid;
} __attribute__ ((packed)) partition_data_t;

#define FWTYPE_UNKNOWN	0
#define FWTYPE_UBNT	1
#define FWTYPE_OPEN	2
#define FWTYPE_OLD	3


#define MAX_PARTITIONS 8

typedef struct fw {
	unsigned char version[128];
	unsigned char type;
	partition_data_t parts[MAX_PARTITIONS];
	unsigned int part_count;
} fw_t;

#define __DEBUG 1

static int
fw_check_header(fw_t* fw, const header_t* h) {
	unsigned long crc;
	unsigned int len = sizeof(header_t) - 2 * sizeof(unsigned long);

	fw->type = FWTYPE_UNKNOWN;
	if (strncmp(h->magic, "UBNT", 4) == 0) {
		fw->type = FWTYPE_UBNT;
	} else if (strncmp(h->magic, "GEOS", 4) == 0) {
		fw->type = FWTYPE_OLD;
	} else if (strncmp(h->magic, "OPEN", 4) == 0) {
		fw->type = FWTYPE_OPEN;
	}

	if (fw->type == FWTYPE_UNKNOWN)
		return -1;

	crc = crc32(0L, (const unsigned char*)h, len);
	if (htonl(crc) != h->crc)
		return -1;

#ifdef RECOVERY_FWVERSION_ARCH
	{
		const char* arch;
		int i, archlen = strlen(RECOVERY_FWVERSION_ARCH);
		arch = h->version;
		for (i = 0; (*arch  != '.') && (i < sizeof(h->version)); ++i, ++arch);

		// no arch in version?
		if (i >= (sizeof(h->version) - archlen))
			return -1;

		arch++;

		if (strncmp(arch, RECOVERY_FWVERSION_ARCH, archlen)) {
			return -1;
		}
	}
#endif

	return 0;
}

static fw_t fw;

int
fw_has_part(void* base, int size, const char* partname) {
	part_t* p;
	int len = strlen(partname);

	p = (part_t*)((unsigned char*)base + sizeof(header_t));
	while (strncmp(p->magic, "END.", 4)) {
		if ((strncmp(p->magic, "PART", 4) == 0) &&
				(len == strlen(p->name)) &&
				(strncmp(p->name, partname, len) == 0)) {
			return 0;
		}
		p = (part_t*)((unsigned char*)p + sizeof(part_t) + ntohl(p->length) + sizeof(part_crc_t));
	}

	return 1;
}

extern int mtdparts_init(void); /* from common/cmd_jffs2.c */
extern int find_mtd_part(const char *id, struct mtd_device **dev,
        u8 *part_num, struct part_info **part); /* common/cmd_jffs2.c */

extern int ubnt_auth_image(uint32_t *sig, uint8_t *data, uint32_t len);

int
ubnt_fw_check(void* base, int size) {
	unsigned long crc;
	header_t* h = (header_t*)base;
	part_t* p;
	int rc = 0;
	int i;
	signature_t* sig;

    partition_data_t* fwp;
    struct part_info *prt;
    struct mtd_device *cdev;
    u8 pnum;
    int match_part_count;

	if (size < sizeof(header_t))
		return -1;

	memset(&fw, 0, sizeof(fw_t));

	rc = fw_check_header(&fw, h);
	if (rc) {
		return -2;
	}

	printf("Firmware Version: %s\n", h->version);

	p = (part_t*)((unsigned char*)base + sizeof(header_t));

	i = 0;

	while (strncmp(p->magic, "END.", 4)
          )
    {
#ifdef __DEBUG2
		printf(" Partition (%c%c%c%c): %s [%lu]\n",
				p->magic[0], p->magic[1], p->magic[2], p->magic[3],
				p->name, ntohl(p->id));
		printf("  Partition size: 0x%X\n", ntohl(p->part_size));
		printf("  Data length: %lu\n", ntohl(p->length));
#endif

		if ((strncmp(p->magic, "PART", 4) == 0) && (i < MAX_PARTITIONS)) {
			partition_data_t* fwp = &(fw.parts[i]);
			fwp->header = p;
			fwp->data = (unsigned char*)p + sizeof(part_t);
			fwp->signature = (part_crc_t*)(fwp->data + ntohl(p->length));
			crc = crc32(0L, (unsigned char*)p, ntohl(p->length) + sizeof(part_t));
			fwp->valid = (htonl(crc) == fwp->signature->crc);
			++i;
#ifdef __DEBUG2
			printf("  CRC Valid: %s\n", fw_data[i].valid ? "true" : "false");
#endif
		}

		p = (part_t*)((unsigned char*)p + sizeof(part_t) + ntohl(p->length) + sizeof(part_crc_t));
	}
	fw.part_count = i;

#ifdef __DEBUG2
	printf("Found %d parts\n", fw.part_count);
#endif
	sig = (signature_t*)p;
	if (strncmp(sig->magic, "END.", 4) != 0) {
#ifdef __DEBUG
		printf("Bad signature!\n");
#endif
		return -3;
	}

        if (!strncmp(sig->magic, "END.", 4)) {
	  crc = crc32(0L, (unsigned char*)base, (char*)sig - (char*)base);
#ifdef __DEBUG2
	  printf("  Signature CRC: 0x%08X\n", sig->crc);
	  printf("  Calculated CRC: 0x%08X\n", htonl(crc));
#endif
	  if (htonl(crc) != sig->crc) {
#ifdef __DEBUG
		printf("Bad signature CRC!\n");
#endif
		return -4;
	  }
        }

    match_part_count = 0;
    if (!mtdparts_init()) {
        for (i = 0; i < fw.part_count; ++i) {
            fwp = &(fw.parts[i]);
            if ((fwp->valid != 0) && find_mtd_part(fwp->header->name, &cdev, &pnum, &prt)) {
                match_part_count++;
	    }
        }
    }

    if (match_part_count == 0) return -5;

	return 0;
}

extern flash_info_t flash_info[];	/* info for FLASH chips */

#define I_WANT_TO_WRITE_TO_FLASH 1
#define FW_CFG_PART_NAME	"cfg"	/* Reset this partition on user request */

extern int flash_status_display(int);
/*
 * We have joy, we have fun, we have seasons in the sun...
 */
static int
ubnt_update_fw(uchar *base, unsigned size)
{
	int i = 0;
	partition_data_t* p;
	ulong flash_base = CFG_FLASH_BASE;

	struct part_info *prt;
	struct mtd_device *cdev;
	u8 pnum;

	int rc;
	unsigned long length;
	unsigned long first_addr, last_addr;

#ifdef CONFIG_SHOW_BOOT_PROGRESS
	show_boot_progress(-7);  /* fw update in progress */
#endif

	flash_info_t* flinfo = &flash_info[0];
	unsigned int flash_block_size = flinfo->size / flinfo->sector_count;

    flash_status_display(1);
	printf("Setting U-Boot environment variables\n");
	/*
	 * Default values for boot-critical variables
	 */
	setenv("mtdparts", MTDPARTS_DEFAULT);
	setenv("bootcmd", CONFIG_BOOTCOMMAND);
	/* If the firmware has ttyS0 enabled, leave it enabled. */
	//if (0 != strcmp( getenv( "bootargs"),  CONFIG_BOOTARGS_TTYS0 )) {
		setenv("bootargs", CONFIG_BOOTARGS);
	//}

#define _MK_STR(x) #x
#define MK_STR(x) _MK_STR(x)

	setenv("ipaddr", MK_STR(CONFIG_IPADDR));

	//saveenv();

	if (mtdparts_init()) {
		printf("Warning: mtdparts_init() failed! (check mtdparts variable?)\n");
        flash_status_display(0);
		return 1;
	}

#if 0
	/* Reset cfg part on user request */
	if (ResetCfgToDefaults) {
		/* lookup cfg in the U-Boot mtdparts */
		if (!find_mtd_part(FW_CFG_PART_NAME, &cdev, &pnum, &prt)) {
			printf("Warning: partition %s not found on flash memory! Skipped.\n",
				FW_CFG_PART_NAME);
		} else {
			printf("Clearing partition '%s':\n", FW_CFG_PART_NAME);
			first_addr = flinfo->start[0] + prt->offset;
			last_addr = first_addr + prt->size - 1;
			if (last_addr < first_addr)
				last_addr = first_addr;
			printf ("\terasing range 0x%08lX..0x%08lX: ", first_addr, last_addr);
#ifdef I_WANT_TO_WRITE_TO_FLASH
			rc = flash_sect_erase(first_addr, last_addr);

			if (rc) {
				printf("Error occured while erasing partition '%s'!: %d\n",
						FW_CFG_PART_NAME, rc);
			}
#endif


		}
	}
#endif

	for (i = 0; i < fw.part_count; ++i) {

		p = &(fw.parts[i]);

		if (p->valid == 0) {
			continue;
		}

		/* lookup p->header->name in the U-Boot mtdparts */
		if (!find_mtd_part(p->header->name, &cdev, &pnum, &prt)) {
			printf("Warning: partition %s not found on flash memory! Skipped.\n",
				p->header->name);
			continue;
		}

		if (!TftpServerOverwriteBootloader) {
			if (!strcmp("u-boot", p->header->name)) {
				printf("Will not overwrite u-boot partition! Skipped.\n");
				continue;
			}
		}

		length = (ntohl(p->header->length) + flash_block_size - 1) / flash_block_size;
		length *= flash_block_size;

		printf("Copying partition '%s' to flash memory:\n", p->header->name);

		first_addr = flinfo->start[0] + prt->offset;
		last_addr = first_addr + length - 1;
		if (last_addr < first_addr)
			last_addr = first_addr;
#ifndef UBNT_APP
		printf ("\terasing range 0x%08lX..0x%08lX: ", first_addr, last_addr);
#ifdef I_WANT_TO_WRITE_TO_FLASH
		rc = flash_sect_erase(first_addr, last_addr);

		if (rc) {
			printf("Error occured while erasing partition '%s'!: %d\n",
					p->header->name, rc);
		}
#endif

#ifdef I_WANT_TO_WRITE_TO_FLASH
		printf("\twriting to address 0x%08x, length 0x%08x ...\n",
				flash_base + prt->offset, length);
		rc = write_buff(flash_info, (uchar *)p->data, flash_base + prt->offset, length);

		if (rc) {
			printf("Error occured while flashing partition '%s'! (%d)\n", p->header->name, rc);
            flash_status_display(0);
			return 1;
		}
#else
		printf("FAKE flash_program(%p, %p, 0x%08X)\n", (void*)0, p->data, length);
#endif
#else  /* UAP_APP */
#ifdef I_WANT_TO_WRITE_TO_FLASH
                if ((rc=ubnt_fw_flash(p->data, flash_base + prt->offset, last_addr, length))) {
                    printf("Error occured while flashing partition '%s'! (%d)\n", p->header->name, rc);
                    flash_status_display(0);
                    return 1;
                }

#else
		printf("FAKE flash_program(%p, %p, 0x%08X)\n", (void*)0, p->data, length);
#endif
#endif /* UAP_APP */
        }
	puts("\nFirmware update complete.\n");

        flash_status_display(0);
	return 0;
}

static void TftpServer (void);
static void TftpTimeout (void);

/**********************************************************************/

static void
TftpServer (void)
{
	volatile uchar *	pkt;
	volatile uchar *	xp;
	int			len = 0;
	volatile ushort *s;
    int need_send = 0;

	/*
	 *	We will always be sending some sort of packet, so
	 *	cobble together the packet headers now.
	 */
	pkt = NetTxPacket + NetEthHdrSize() + IP_HDR_SIZE;

	switch (TftpServerState) {

	case TFTP_SRV_STATE_RRQ:
		xp = pkt;
		s = (ushort *)pkt;
		*s++ = htons(TFTP_RRQ);
		pkt = (uchar *)s;
		strcpy ((char *)pkt, tftp_filename);
		pkt += strlen(tftp_filename) + 1;
		strcpy ((char *)pkt, "octet");
		pkt += 5 /*strlen("octet")*/ + 1;
		strcpy ((char *)pkt, "timeout");
		pkt += 7 /*strlen("timeout")*/ + 1;
		sprintf((char *)pkt, "%d", TIMEOUT);
#ifdef ET_DEBUG
		printf("send option \"timeout %s\"\n", (char *)pkt);
#endif
		pkt += strlen((char *)pkt) + 1;
		len = pkt - xp;
        need_send = 1;
		break;

	case TFTP_SRV_STATE_DATA_SEND: /* send data to client */
		if ((TftpBlock - 1) * TFTP_BLOCK_SIZE > TftpFileSize) {
			printf("Client requested data block outside available data! (#%d)\n",
				TftpBlock);
			return;
		}
		xp = pkt;
		s = (ushort *)pkt;
		*s++ = htons(TFTP_DATA);
		*s++ = htons(TftpBlock);
		if ((TftpBlock - 1) * TFTP_BLOCK_SIZE >= TftpFileSize)
			len = TftpFileSize % TFTP_BLOCK_SIZE;
		else
			len = TFTP_BLOCK_SIZE;
		memmove((void *)s,(void *)(TftpFileAddr + (TftpBlock - 1) * TFTP_BLOCK_SIZE), len);
		pkt = (uchar *)s;
		len += pkt - xp;
        need_send = 1;
		break;

	case TFTP_SRV_STATE_DATA: /* send ACK for received data */
	case TFTP_SRV_STATE_OACK:
		xp = pkt;
		s = (ushort *)pkt;
		*s++ = htons(TFTP_ACK);
		*s++ = htons(TftpBlock);
		pkt = (uchar *)s;
		len = pkt - xp;
        need_send = 1;
		break;

	case TFTP_SRV_STATE_TOO_LARGE:
		xp = pkt;
		s = (ushort *)pkt;
		*s++ = htons(TFTP_ERROR);
		*s++ = htons(3);
		pkt = (uchar *)s;
		strcpy ((char *)pkt, "File too large");
		pkt += 14 /*strlen("File too large")*/ + 1;
		len = pkt - xp;
        need_send = 1;
		break;

	case TFTP_SRV_STATE_BAD_MAGIC:
		xp = pkt;
		s = (ushort *)pkt;
		*s++ = htons(TFTP_ERROR);
		*s++ = htons(2);
		pkt = (uchar *)s;
		strcpy ((char *)pkt, "File has bad magic");
		pkt += 18 /*strlen("File has bad magic")*/ + 1;
		len = pkt - xp;
        need_send = 1;
		break;
	case TFTP_SRV_STATE_BADFW:
		xp = pkt;
		s = (ushort *)pkt;
		*s++ = htons(TFTP_ERROR);
		*s++ = htons(2);
		pkt = (uchar *)s;
#define TFTP_SRV_FW_CHECK_FAIL	"Firmware check failed"
		strcpy ((char *)pkt, TFTP_SRV_FW_CHECK_FAIL);
		pkt += strlen(TFTP_SRV_FW_CHECK_FAIL) + 1;
		len = pkt - xp;
        need_send = 1;
		break;
	}

    if (need_send)
        NetSendUDPPacket(NetServerEther, TftpOurClientIP, TftpClientPort, TftpOurPort, len);
}

extern int do_reset (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]);

static void
TftpServerHandler (uchar * pkt, unsigned dest, unsigned src, unsigned len)
{
	ushort proto;
	ushort *s;
	//volatile struct tftphdr *hdr = (volatile struct tftphdr *)pkt;
	//int i;
	unsigned char *ip;

	/* for find_mtd_part() */
	struct part_info *prt;
	struct mtd_device *cdev;
	u8 pnum;
	flash_info_t* flinfo = &flash_info[0];

	ip = (unsigned char *)pkt;
	ip -= IP_HDR_SIZE;

#if 0
	printf("-- port from %d to %d, len = %d, TFTP opcode = %d\n", src, dest, len, hdr->opcode);

	for (i=0; i<len+IP_HDR_SIZE; i++) {
		if ((i % 16) == 0) puts("\n");
		printf("%02x ", ip[i]);
	}
	puts("\n");

#endif
	if ((TftpServerState != TFTP_SRV_STATE_WRQ) && (TftpServerState != TFTP_SRV_STATE_RRQ) &&
			(TftpServerState != TFTP_SRV_STATE_CONNECT) && TftpClientPort &&
			(src != TftpClientPort)) {
		return;
	}
	if (dest != TftpOurPort) {
		return;
	}

	if (len < 2) {
		return;
	}

	len -= 2;
	/* warning: don't use increment (++) in ntohs() macros!! */
	s = (ushort *)pkt;
	proto = *s++;
	pkt = (uchar *)s;

	switch (ntohs(proto)) {
	case TFTP_WRQ:
		printf("\nReceiving file from " STR_IP":%d\n",
			ip[12], ip[13], ip[14], ip[15], src);

        led_tick = 0;
        unifi_set_led((led_tick & 0x01) + 1);
		TftpServerState = TFTP_SRV_STATE_DATA;
		TftpClientPort = src;
		TftpBlock = 0;
		TftpLastBlock = 0;
		TftpBlockWrap = 0;
		TftpBlockWrapOffset = 0;

		memcpy(&TftpOurClientIP, &ip[12], sizeof(TftpOurClientIP));

		TftpServer (); /* Send ACK for block 0 */
		break;

	case TFTP_RRQ:
#if 0 /* ignore "GET" requests from client */
		printf("\nIgnoring GET request from " STR_IP":%d\n",
			ip[12], ip[13], ip[14], ip[15], src);
		return;
#else
		/* TBD: check filename */
		strncpy(tftp_filename, (char *)s, sizeof(tftp_filename));

		/* file for each partition (you can use something like "GET u-boot-env") */
		if (!mtdparts_init() && find_mtd_part(tftp_filename, &cdev, &pnum, &prt)) {
			TftpFileAddr = flinfo->start[0] + prt->offset;
			TftpFileSize = prt->size;
		}
		else if (!strcmp("flash_dump", tftp_filename)) {
			/* send whole flash dump */
			TftpFileAddr = flinfo->start[0];
			TftpFileSize = flinfo->size;
		} else {
			/* just ignore */
			printf("Ignoring GET attempt for unknown file: <%s>\n", tftp_filename);
			return;
		}

		TftpServerState = TFTP_SRV_STATE_DATA_SEND;
		printf("\nSending <%s> (0x%08X@0x%08X) to " STR_IP":%d\n",
			tftp_filename, TftpFileSize, TftpFileAddr,
			ip[12], ip[13], ip[14], ip[15], src);

		memcpy(&TftpOurClientIP, &ip[12], sizeof(TftpOurClientIP));

		TftpClientPort = src;
		TftpBlock = 1;

		TftpServer (); /* Send DATA */
#endif
		break;

	case TFTP_ACK:
		/* ack from client - send next data block */
		if (TftpServerState == TFTP_SRV_STATE_DATA_SEND) {
			/* update TftpBlock */
			TftpBlock = ntohs(*(ushort *)pkt);
			/* move to next block ( if this is not the last one ) */
			if ((TftpBlock * TFTP_BLOCK_SIZE) <= TftpFileSize)
				TftpBlock++;
			else {
				/* this is ACK to the last DATA packet */
				printf("File transfer completed successfuly.\n");
				TftpServerState = TFTP_SRV_STATE_CONNECT;
			}
			NetSetTimeout (TIMEOUT * CFG_HZ, TftpTimeout);
			TftpServer (); /* Send DATA */
		}
		break;

	default:
		break;

	case TFTP_OACK:
#ifdef ET_DEBUG
		printf("Got OACK: %s %s\n", pkt, pkt+strlen(pkt)+1);
#endif
		TftpServerState = TFTP_SRV_STATE_OACK;
		TftpClientPort = src;
		TftpServer (); /* Send ACK */
		break;

	case TFTP_DATA:
		if (len < 2)
			return;
		len -= 2;
		TftpBlock = ntohs(*(ushort *)pkt);

		/*
		 * RFC1350 specifies that the first data packet will
		 * have sequence number 1. If we receive a sequence
		 * number of 0 this means that there was a wrap
		 * around of the (16 bit) counter.
		 */
		if (TftpBlock == 0) {
			TftpBlockWrap++;
			TftpBlockWrapOffset += TFTP_BLOCK_SIZE * TFTP_SEQUENCE_SIZE;
			printf ("\n\t %lu MB reveived\n\t ", TftpBlockWrapOffset>>20);
		} else {
#if 1
			/* every half-second or on last block */
			if (((get_timer(TftpLastTimer)) > (CFG_HZ/2)) || (len < TFTP_BLOCK_SIZE)) {
				printf("Received %d bytes\r",
						(TftpBlock -1 ) * TFTP_BLOCK_SIZE + len);
                led_tick++;
                unifi_set_led((led_tick & 0x01) + 1);
				TftpLastTimer = get_timer(0);
			}
#else
			if (((TftpBlock - 1) % 10) == 0) {
				putc ('#');
			} else if ((TftpBlock % (10 * HASHES_PER_LINE)) == 0) {
				puts ("\n\t ");
			}
#endif
		}

#ifdef ET_DEBUG
		if (TftpServerState == TFTP_SRV_STATE_RRQ) {
			puts ("Server did not acknowledge timeout option!\n");
		}
#endif
		if (TftpServerState == TFTP_SRV_STATE_WRQ ||
				TftpServerState == TFTP_SRV_STATE_RRQ ||
				TftpServerState == TFTP_SRV_STATE_OACK) {
			/* first block received */
			TftpServerState = TFTP_SRV_STATE_DATA;
			TftpClientPort = src;
			TftpLastBlock = 0;
			TftpBlockWrap = 0;
			TftpBlockWrapOffset = 0;

			if (TftpBlock != 1) {	/* Assertion */
				printf ("\nTFTP error: "
					"First block is not block 1 (%ld)\n"
					"Starting again\n\n",
					TftpBlock);
				NetStartAgain ();
				break;
			}
		}

		if (TftpBlock == TftpLastBlock) {
			/*
			 *	Same block again; ignore it.
			 */
			break;
		}

		TftpLastBlock = TftpBlock;
		NetSetTimeout (TIMEOUT * CFG_HZ, TftpTimeout);

		store_block (TftpBlock - 1, pkt + 2, len);

		if (len < TFTP_BLOCK_SIZE) {
			uchar *fw = (uchar *)load_addr;
			unsigned total_len = (TftpLastBlock - 1) * TFTP_BLOCK_SIZE +
				TftpBlockWrapOffset + len;
			int ret;

			/*
			 *	We received the whole thing.
			 *	NOTE: last ACK still pending, don't forget to send it!!!
			 */
			putc('\n');

			/* Firmware file is uploaded to load_addr, size <len>.
			 * Check headers and CRC.
			 * If everything is ok - write FW to flash memory and reboot.
			 * Oh, don't forget to send last ACK!
			 * Else - return (TFTP server should restart).
			 */

                        unifi_set_led(0);
#ifndef UBNT_APP
			if ((ret = ubnt_fw_check(fw,total_len))) {
				printf("Firmware check failed! (%d)\n", ret);
				/* send last ACK with error message */
				TftpServerState = TFTP_SRV_STATE_BADFW;
			}
#endif /* UBNT_APP */
			/* Last ACK */
			TftpServer();
#ifndef UBNT_APP
			if (!ret) {
				TftpServerState = TFTP_SRV_STATE_FWUPDATE;
				if (!ubnt_update_fw(fw, total_len)) {
				    do_reset(NULL, 0, 0, NULL);
				}
			}
#else /* UBNT_APP */
			TftpServerState = TFTP_SRV_STATE_FWUPDATE;
                        printf("TFTP Transfer Complete.\n");
			NetState = NETLOOP_SUCCESS;
                        break;
#endif /* UBNT_APP */

			/*
			 * If we are here - then something went wrong with urescue,
			 * wait for firmware (again)...
			 */

			NetStartAgain ();
			/* Use this to return to U-Boot cmd prompt
			 * NetState = NETLOOP_SUCCESS; */
		} else {
			/*
			 *	Acknowledge the block just received, which will prompt
			 *	the server for the next one.
			 */
			TftpServer ();
		}
		break;

	case TFTP_ERROR:
		printf ("\nTFTP error: '%s' (%d)\n",
					pkt + 2, ntohs(*(ushort *)pkt));
		puts ("Starting again\n\n");
		NetStartAgain ();
		break;
	}
}

static int beacon_ready = 0;
static uint8_t beacon_buf[256];
static uint8_t beacon_len = 0;
static IPaddr_t beacon_ip = 0x00000000;

static void
UbntBeaconSend (void)
{
	volatile uchar *	pkt;
	volatile uchar *	xp;
	int			len = 0;
    volatile ushort *lenp;
    uchar bcastether[6];
    IPaddr_t tmp_ip;

	/*
	 *	We will always be sending some sort of packet, so
	 *	cobble together the packet headers now.
	 */
	pkt = NetTxPacket + NetEthHdrSize() + IP_HDR_SIZE;

    if ((beacon_ready == 0) || (beacon_ip != NetOurIP)) {
        beacon_len = prepare_beacon(beacon_buf);
        if (beacon_len == 0) {
            return;
        } else {
            beacon_ip = NetOurIP;
            beacon_ready = 1;
        }
    }

    memcpy(pkt, beacon_buf, beacon_len);
    memset(bcastether, 0xff, 6);
	NetSendUDPPacket(bcastether, 0xffffffff, 10001, 10000, beacon_len);

}

static uchar ticker_cnt = 0;
static uchar ticker[] = {'-', '\\', '|', '/'};

static void
TftpTimeout (void)
{
	/* In case this gets called while fwupdate is in progress */
	if (TftpServerState == TFTP_SRV_STATE_FWUPDATE) {
		return;
	}

	if (TftpServerState == TFTP_SRV_STATE_CONNECT) {
#ifdef CONFIG_SHOW_BOOT_PROGRESS
		show_boot_progress((ticker_cnt % 2) ? -5 : -6);
#endif
        if ((led_tick % 3) == 0) UbntBeaconSend();
        unifi_set_led((led_tick % 3));
        led_tick++;
		/* we are waiting for new connection */
		putc(ticker[ticker_cnt]);
		putc('\b');
		if (++ticker_cnt >= (sizeof(ticker)/sizeof(ticker[0])))
			ticker_cnt = 0;

		NetSetTimeout (TIMEOUT * CFG_HZ, TftpTimeout);
		TftpServer ();
		return;
	}

	if (++TftpTimeoutCount > TIMEOUT_COUNT) {
		puts ("\nRetry count exceeded; starting again\n");
		NetStartAgain ();
	} else {
		/* retry */
		puts ("T ");
		NetSetTimeout (TIMEOUT * CFG_HZ, TftpTimeout);
		TftpServer ();
	}
}

/* from ap93.c */
//extern int ar7240_reset_button(void);

void
TftpServerStart (void)
{
	//u8 reset_btn_count = 0;

	puts("Starting TFTP server...\n");

#if defined(CONFIG_NET_MULTI)
	printf ("Using %s ", eth_get_name());
#endif
	puts ("(");	print_IPaddr (NetOurIP); puts ("), address: ");
	printf ("0x%lx\n", load_addr);

	TftpServerState = TFTP_SRV_STATE_CONNECT;
	TftpOurPort = WELL_KNOWN_PORT;
	TftpBlock = 0;
	TftpTimeoutCount = 0;
	TftpClientPort = 0;
	TftpLastTimer = get_timer(0);

#if 0
#define RESET_BTN_TIME	10
	show_boot_progress(-8); /*clear progress leds*/
	/* Check if user wants configuration reset (holds Reset during tftp server startup).
	 * Aproximate arrival time (counting from power-on) to this function is 6 seconds,
	 * here we will wait for 5 more seconds to see if user wants to clear cfg.
	 */
	if (ar7240_reset_button()) {
		/* wait for 5 seconds */
		while ((reset_btn_count++ < RESET_BTN_TIME) && ar7240_reset_button()) {
			/* blink LEDs */
			show_boot_progress(-9); /*light next reset progress led*/

			udelay(1000 * 1000);
		}
	}

	if (reset_btn_count >= RESET_BTN_TIME) {
		printf ("Will reset device configuration (Reset button active after %d seconds).\n", RESET_BTN_TIME);
		ResetCfgToDefaults = 1;
	}
#endif

	puts ("Waiting for connection: *\b");

	NetSetTimeout (TIMEOUT * CFG_HZ, TftpTimeout);
	NetSetHandler (TftpServerHandler);

	/* zero out server ether in case the server ip has changed */
	memset(NetServerEther, 0, 6);
}

#endif /* CFG_CMD_NET */
