/*
 *	TFTP server.
 *
 *	Copyright 2009 Ubiquiti
 */

#ifndef __TFTP_SERVER_H__
#define __TFTP_SERVER_H__

#define RECOVERY_PORT 10002

#define UBNT_STR "\x8e\x46\x93\x78"
typedef struct {
	uint8_t ubnt[4]; // magic
	uint8_t ver; // 1
#define UBNT_REC_VERSION 1
	uint8_t cmd;
	uint8_t _res[2];
#define UBNT_REC_CMD_URESCUE	1
#define UBNT_REC_CMD_ANNOUNCE	2
	union {
		struct { // UBNT_REC_CMD_URESCUE
			uint32_t ipaddr;
			uint32_t netmask;
		} urescue;
		struct { // UBNT_REC_CMD_ANNOUNCE
			uint32_t ipaddr;
		} announce;
	} u;
} recovery_t;

/**********************************************************************/
/*
 *	Global functions and variables.
 */

/* tftp_server.c */
extern void	TftpServerStart (void);	/* Start TFTP server */

/**********************************************************************/

#endif
