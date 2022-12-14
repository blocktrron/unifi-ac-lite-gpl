#ifndef __LINUX_MROUTE_H
#define __LINUX_MROUTE_H

#include <linux/in.h>
#include <linux/pim.h>
#include <net/sock.h>
#include <uapi/linux/mroute.h>

#ifdef CONFIG_IP_MROUTE
static inline int ip_mroute_opt(int opt)
{
	return (opt >= MRT_BASE) && (opt <= MRT_MAX);
}
#else
static inline int ip_mroute_opt(int opt)
{
	return 0;
}
#endif

#ifdef CONFIG_IP_MROUTE
extern int ip_mroute_setsockopt(struct sock *, int, char __user *, unsigned int);
extern int ip_mroute_getsockopt(struct sock *, int, char __user *, int __user *);
extern int ipmr_ioctl(struct sock *sk, int cmd, void __user *arg);
extern int ipmr_compat_ioctl(struct sock *sk, unsigned int cmd, void __user *arg);
extern int ip_mr_init(void);
#else
static inline
int ip_mroute_setsockopt(struct sock *sock,
			 int optname, char __user *optval, unsigned int optlen)
{
	return -ENOPROTOOPT;
}

static inline
int ip_mroute_getsockopt(struct sock *sock,
			 int optname, char __user *optval, int __user *optlen)
{
	return -ENOPROTOOPT;
}

static inline
int ipmr_ioctl(struct sock *sk, int cmd, void __user *arg)
{
	return -ENOIOCTLCMD;
}

static inline int ip_mr_init(void)
{
	return 0;
}
#endif

struct vif_device {
	struct net_device 	*dev;			/* Device we are using */
	unsigned long	bytes_in,bytes_out;
	unsigned long	pkt_in,pkt_out;		/* Statistics 			*/
	unsigned long	rate_limit;		/* Traffic shaping (NI) 	*/
	unsigned char	threshold;		/* TTL threshold 		*/
	unsigned short	flags;			/* Control flags 		*/
	__be32		local,remote;		/* Addresses(remote for tunnels)*/
	int		link;			/* Physical interface index	*/
};

#define VIFF_STATIC 0x8000

struct mfc_cache {
	struct list_head list;
	__be32 mfc_mcastgrp;			/* Group the entry belongs to 	*/
	__be32 mfc_origin;			/* Source of packet 		*/
	vifi_t mfc_parent;			/* Source interface		*/
	int mfc_flags;				/* Flags on line		*/

	union {
		struct {
			unsigned long expires;
			struct sk_buff_head unresolved;	/* Unresolved buffers		*/
		} unres;
		struct {
			unsigned long last_assert;
			int minvif;
			int maxvif;
			unsigned long bytes;
			unsigned long pkt;
			unsigned long wrong_if;
			unsigned char ttls[MAXVIFS];	/* TTL thresholds		*/
		} res;
	} mfc_un;
	struct rcu_head	rcu;
};

#define MFC_STATIC		1
#define MFC_NOTIFY		2

#define MFC_LINES		64

#ifdef __BIG_ENDIAN
#define MFC_HASH(a,b)	(((((__force u32)(__be32)a)>>24)^(((__force u32)(__be32)b)>>26))&(MFC_LINES-1))
#else
#define MFC_HASH(a,b)	((((__force u32)(__be32)a)^(((__force u32)(__be32)b)>>2))&(MFC_LINES-1))
#endif		

struct rtmsg;
extern int ipmr_get_route(struct net *net, struct sk_buff *skb,
			  __be32 saddr, __be32 daddr,
			  struct rtmsg *rtm, int nowait, u32 portid);
#define IPMR_MFC_EVENT_UPDATE   1
#define IPMR_MFC_EVENT_DELETE   2

/*
 * Callback to registered modules in the event of updates to a multicast group
 */
typedef void (*ipmr_mfc_event_offload_callback_t)(__be32 origin, __be32 group,
						  u32 max_dest_dev,
						  u32 dest_dev_idx[],
						  u8 op);

/*
 * Register the callback used to inform offload modules when updates occur to
 * MFC. The callback is registered by offload modules
 */
extern bool ipmr_register_mfc_event_offload_callback(
			ipmr_mfc_event_offload_callback_t mfc_offload_cb);

/*
 * De-Register the callback used to inform offload modules when updates occur
 * to MFC
 */
extern void ipmr_unregister_mfc_event_offload_callback(void);

/*
 * Find the destination interface list, given a multicast group and source
 */
extern int ipmr_find_mfc_entry(struct net *net, __be32 origin, __be32 group,
				 u32 max_dst_cnt, u32 dest_dev[]);

/*
 * Out-of-band multicast statistics update for flows that are offloaded from
 * Linux
 */
extern int ipmr_mfc_stats_update(struct net *net, __be32 origin, __be32 group,
				 u64 pkts_in, u64 bytes_in,
				 u64 pkts_out, u64 bytes_out);
#endif
