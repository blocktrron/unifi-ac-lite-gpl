    /* Copyright Ubiquiti Networks Inc.
    ** All Right Reserved.
    */


#ifndef _UBNT_EXEC_H_
#define _UBNT_EXEC_H_


typedef struct beacon_packet {
    int ret; /* Note: Overlaps with ret entry in ubnt_exec union */
    uchar *ptr;
    int len;
} beacon_pkt_t;

typedef union ubnt_exec {
    int ret;
    beacon_pkt_t beacon_pkt;
} ubnt_exec_t;

int ubnt_app_load(void);
int ubnt_app_init(void);

#endif
