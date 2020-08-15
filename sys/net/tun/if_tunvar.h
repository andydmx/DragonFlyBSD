/*-
 * Copyright (c) 1998 Brian Somers <brian@Awfulhak.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/net/if_tunvar.h,v 1.7 2000/01/23 01:47:12 brian Exp $
 */

#ifndef _NET_IF_TUNVAR_H_
#define _NET_IF_TUNVAR_H_

struct tun_softc {
	cdev_t		tun_dev;	/* device */

	u_short		tun_flags;	/* misc flags */
#define	TUN_OPEN	0x0001
#define	TUN_INITED	0x0002
#define	TUN_RCOLL	0x0004
#define	TUN_IASET	0x0008
#define	TUN_DSTADDR	0x0010		/* unused */
#define	TUN_LMODE	0x0020
#define	TUN_RWAIT	0x0040
#define	TUN_ASYNC	0x0080
#define	TUN_IFHEAD	0x0100
#define	TUN_CLONE	0x0200
#define	TUN_MANUALMAKE	0x0400
#define	TUN_READY	(TUN_OPEN | TUN_INITED)

	pid_t		 tun_pid;	/* PID of process to open */
	struct ifnet	*tun_ifp;	/* the interface */
	struct sigio	*tun_sigio;	/* information for async I/O */
	struct kqinfo	 tun_rkq;	/* read select/poll/kq */
	struct kqinfo	 tun_wkq;	/* write select/poll/kq (not used) */

	SLIST_ENTRY(tun_softc)	tun_link;	/* local tun list */
};

#endif /* !_NET_IF_TUNVAR_H_ */
