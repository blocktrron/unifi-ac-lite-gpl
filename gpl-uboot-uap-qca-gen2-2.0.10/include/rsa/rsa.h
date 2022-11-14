/*
 * Copyright (c) 2013, Google Inc.
 *
 * (C) Copyright 2008 Semihalf
 *
 * (C) Copyright 2000-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _RSA_H
#define _RSA_H

#if 0
#include <config.h>
#include <common.h>
#include <malloc.h>
#include <net.h>
#include <command.h>
#endif
#include <asm/io.h>
#include <asm/addrspace.h>
#include <asm/types.h>

#include <rsa/be_byteshift.h>

/**
 * struct rsa_public_key - holder for a public key
 *
 * An RSA public key consists of a modulus (typically called N), the inverse
 * and R^2, where R is 2^(# key bits).
 */
struct rsa_public_key {
        uint len;               /* Length of modulus[] in number of uint32_t */
        uint32_t n0inv;         /* -1 / modulus[0] mod 2^32 */
        uint32_t *modulus;      /* modulus as little endian array */
        uint32_t *rr;           /* R^2 as little endian array */
};
#endif
