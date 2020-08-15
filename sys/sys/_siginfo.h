/*-
 * Copyright (c) 2019 The DragonFly Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of The DragonFly Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific, prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYS__SIGINFO_H_
#define	_SYS__SIGINFO_H_

#include <sys/cdefs.h>
#include <machine/stdint.h>

#ifndef _PID_T_DECLARED
typedef	__pid_t		pid_t;
#define	_PID_T_DECLARED
#endif

#ifndef _UID_T_DECLARED
typedef	__uint32_t	uid_t;		/* user id */
#define	_UID_T_DECLARED
#endif

#if __POSIX_VISIBLE >= 199309 || __XSI_VISIBLE >= 500
union sigval {
	int	sival_int;
	void	*sival_ptr;
};
#endif

#if __POSIX_VISIBLE
/** si_code */
#define	SI_USER		0	/* Sent by kill(2)			*/
#define	SI_QUEUE	-1	/* Sent by the sigqueue(2)		*/
#define	SI_TIMER	-2	/* Generated by expiration of a timer	*/
				/* set by timer_settime(2)		*/
#define	SI_ASYNCIO	-3	/* Generated by completion of an	*/
				/* asynchronous I/O signal		*/
#define	SI_MESGQ	-4	/* Generated by arrival of a message on	*/
				/* an empty message queue		*/
#endif

#if __POSIX_VISIBLE >= 199309 || __XSI_VISIBLE
/*
 * si_code stuff
 */
/* SIGILL */
#define	ILL_ILLOPC	1	/* Illegal opcode			*/
#define	ILL_ILLOPN	2	/* Illegal operand			*/
#define	ILL_ILLADR	3	/* Illegal addressing mode		*/
#define	ILL_ILLTRP	4	/* Illegal trap				*/
#define	ILL_PRVOPC	5	/* Privileged opcode			*/
#define	ILL_PRVREG	6	/* Privileged register			*/
#define	ILL_COPROC	7	/* Coprocessor error			*/
#define	ILL_BADSTK	8	/* Internal stack error			*/

/* SIGFPE */
#define	FPE_INTOVF	1	/* Integer overflow			*/
#define	FPE_INTDIV	2	/* Integer divide by zero		*/
#define	FPE_FLTDIV	3	/* Floating point divide by zero	*/
#define	FPE_FLTOVF	4	/* Floating point overflow		*/
#define	FPE_FLTUND	5	/* Floating point underflow		*/
#define	FPE_FLTRES	6	/* Floating point inexact result	*/
#define	FPE_FLTINV	7	/* Invalid Floating point operation	*/
#define	FPE_FLTSUB	8	/* Subscript out of range		*/

/* SIGSEGV */
#define	SEGV_MAPERR	1	/* Address not mapped to object		*/
#define	SEGV_ACCERR	2	/* Invalid permissions for mapped object*/

/* SIGBUS */
#define	BUS_ADRALN	1	/* Invalid address alignment		*/
#define	BUS_ADRERR	2	/* Non-existent physical address	*/
#define	BUS_OBJERR	3	/* Object specific hardware error	*/

/* SIGTRAP */
#define	TRAP_BRKPT	1	/* Process breakpoint			*/
#define	TRAP_TRACE	2	/* Process trace trap			*/

/* SIGCHLD */
#define	CLD_EXITED	1	/* Child has exited			*/
#define	CLD_KILLED	2	/* Child has terminated abnormally but	*/
				/* did not create a core file		*/
#define	CLD_DUMPED	3	/* Child has terminated abnormally and	*/
				/* created a core file			*/
#define	CLD_TRAPPED	4	/* Traced child has trapped		*/
#define	CLD_STOPPED	5	/* Child has stopped			*/
#define	CLD_CONTINUED	6	/* Stopped child has continued		*/

/* SIGPOLL */
#define	POLL_IN		1	/* Data input available			*/
#define	POLL_OUT	2	/* Output buffers available		*/
#define	POLL_MSG	3	/* Input message available		*/
#define	POLL_ERR	4	/* I/O Error				*/
#define	POLL_PRI	5	/* High priority input available	*/
#define	POLL_HUP	6	/* Device disconnected			*/

typedef	struct __siginfo {
	int	si_signo;		/* signal number */
	int	si_errno;		/* errno association */
	/*
	 * Cause of signal, one of the SI_ macros or signal-specific
	 * values, i.e. one of the FPE_... values for SIGFPE. This
	 * value is equivalent to the second argument to an old-style
	 * FreeBSD signal handler.
	 */
	int	si_code;		/* signal code */
	pid_t	si_pid;			/* sending process */
	uid_t	si_uid;			/* sender's ruid */
	int	si_status;		/* exit value */
	void	*si_addr;		/* faulting instruction */
	union sigval si_value;		/* signal value */
	long	si_band;		/* band event for SIGPOLL */
	int	__spare__[7];		/* gimme some slack */
} siginfo_t;
#endif /* __POSIX_VISIBLE >= 199309 || __XSI_VISIBLE */

#endif /* !_SYS__SIGINFO_H_ */
