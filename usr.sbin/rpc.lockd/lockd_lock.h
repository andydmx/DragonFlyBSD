/*	$NetBSD: lockd_lock.h,v 1.2 2000/06/09 14:00:54 fvdl Exp $	*/
/*	$FreeBSD: src/usr.sbin/rpc.lockd/lockd_lock.h,v 1.4 2002/03/21 22:52:45 alfred Exp $ */
/* $DragonFly$ */

/* Headers and function declarations for file-locking utilities */

struct nlm4_holder	*testlock(struct nlm4_lock *, int);
enum nlm_stats		getlock(nlm4_lockargs *, struct svc_req *, const int);
enum nlm_stats		unlock(nlm4_lock *, const int);
int			lock_answer(int, netobj *, int, int *, int);

void	notify(const char *, const int);

/* flags for testlock, getlock & unlock */
#define LOCK_ASYNC	0x01 /* async version (getlock only) */
#define LOCK_V4		0x02 /* v4 version */
#define LOCK_MON	0x04 /* monitored lock (getlock only) */
#define LOCK_CANCEL	0x08 /* cancel, not unlock request (unlock only) */

/* callbacks from lock_proc.c */
void	transmit_result(int, nlm_res *, struct sockaddr *);
void	transmit4_result(int, nlm4_res *, struct sockaddr *);
CLIENT	*get_client(struct sockaddr *, rpcvers_t);
