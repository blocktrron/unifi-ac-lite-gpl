/*
 *  UBNT Virtual device support for accessing flash  and other HW stuff.
 *
 *  Copyright (C) 2017 Ubiquiti Networks, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#ifndef _ATH79_DEV_UBNT_H
#define _ATH79_DEV_UBNT_H

void ath79_init_ubntdev_pdata(u8 *art_data);
void ath79_register_ubntdev(u8 *art_data);

#endif /* _ATH79_DEV_UBNT_H */
