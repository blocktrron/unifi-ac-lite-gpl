/*
 **************************************************************************
 * Copyright (c) 2016, The Linux Foundation.  All rights reserved.
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 **************************************************************************
 */

#ifndef _PPP_CHANNEL_H_
#define _PPP_CHANNEL_H_
/*
 * Definitions for the interface between the generic PPP code
 * and a PPP channel.
 *
 * A PPP channel provides a way for the generic PPP code to send
 * and receive packets over some sort of communications medium.
 * Packets are stored in sk_buffs and have the 2-byte PPP protocol
 * number at the start, but not the address and control bytes.
 *
 * Copyright 1999 Paul Mackerras.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 * ==FILEVERSION 20000322==
 */

#include <linux/list.h>
#include <linux/skbuff.h>
#include <linux/poll.h>
#include <net/net_namespace.h>

struct ppp_channel;

struct ppp_channel_ops {
	/* Send a packet (or multilink fragment) on this channel.
	   Returns 1 if it was accepted, 0 if not. */
	int	(*start_xmit)(struct ppp_channel *, struct sk_buff *);
	/* Handle an ioctl call that has come in via /dev/ppp. */
	int	(*ioctl)(struct ppp_channel *, unsigned int, unsigned long);
	/* Get channel protocol type, one of PX_PROTO_XYZ or specific to
	 * the channel subtype
	 */
	int (*get_channel_protocol)(struct ppp_channel *);
	/* Get channel protocol version */
	int (*get_channel_protocol_ver)(struct ppp_channel *);
	/* Hold the channel from being destroyed */
	void (*hold)(struct ppp_channel *);
	/* Release hold on the channel */
	void (*release)(struct ppp_channel *);
};

struct ppp_channel {
	void		*private;	/* channel private data */
	const struct ppp_channel_ops *ops; /* operations for this channel */
	int		mtu;		/* max transmit packet size */
	int		hdrlen;		/* amount of headroom channel needs */
	void		*ppp;		/* opaque to channel */
	int		speed;		/* transfer rate (bytes/second) */
	/* the following is not used at present */
	int		latency;	/* overhead time in milliseconds */
};

#ifdef __KERNEL__
/* Call this to obtain the underlying protocol of the PPP channel,
 * e.g. PX_PROTO_OE
 */
extern int ppp_channel_get_protocol(struct ppp_channel *);

/* Call this get protocol version */
extern int ppp_channel_get_proto_version(struct ppp_channel *);

/* Call this to hold a channel */
extern bool ppp_channel_hold(struct ppp_channel *);

/* Call this to release a hold you have upon a channel */
extern void ppp_channel_release(struct ppp_channel *);

/* Release hold on PPP channels */
extern void ppp_release_channels(struct ppp_channel *channels[],
				 unsigned int chan_sz);

/* Hold PPP channels for the PPP device */
extern int ppp_hold_channels(struct net_device *dev,
			     struct ppp_channel *channels[],
			     unsigned int chan_sz);
/* Test if ppp xmit lock is locked */
extern bool ppp_is_xmit_locked(struct net_device *dev);

/* Test if the ppp device is a multi-link ppp device */
extern int ppp_is_multilink(struct net_device *dev);

/* Update statistics of the PPP net_device by incrementing related
 * statistics field value with corresponding parameter
 */
extern void ppp_update_stats(struct net_device *dev, unsigned long rx_packets,
			     unsigned long rx_bytes, unsigned long tx_packets,
			     unsigned long tx_bytes);

/* Called by the channel when it can send some more data. */
extern void ppp_output_wakeup(struct ppp_channel *);

/* Called by the channel to process a received PPP packet.
   The packet should have just the 2-byte PPP protocol header. */
extern void ppp_input(struct ppp_channel *, struct sk_buff *);

/* Called by the channel when an input error occurs, indicating
   that we may have missed a packet. */
extern void ppp_input_error(struct ppp_channel *, int code);

/* Attach a channel to a given PPP unit in specified net. */
extern int ppp_register_net_channel(struct net *, struct ppp_channel *);

/* Attach a channel to a given PPP unit. */
extern int ppp_register_channel(struct ppp_channel *);

/* Detach a channel from its PPP unit (e.g. on hangup). */
extern void ppp_unregister_channel(struct ppp_channel *);

/* Get the channel number for a channel */
extern int ppp_channel_index(struct ppp_channel *);

/* Get the unit number associated with a channel, or -1 if none */
extern int ppp_unit_number(struct ppp_channel *);

/* Get the device name associated with a channel, or NULL if none */
extern char *ppp_dev_name(struct ppp_channel *);

/*
 * SMP locking notes:
 * The channel code must ensure that when it calls ppp_unregister_channel,
 * nothing is executing in any of the procedures above, for that
 * channel.  The generic layer will ensure that nothing is executing
 * in the start_xmit and ioctl routines for the channel by the time
 * that ppp_unregister_channel returns.
 */

#endif /* __KERNEL__ */
#endif
