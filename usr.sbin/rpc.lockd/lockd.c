/*
 * Copyright (c) 1995
 *	A.R. Gordon (andrew.gordon@net-tel.co.uk).  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the FreeBSD project
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ANDREW GORDON AND CONTRIBUTORS ``AS IS'' AND
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
 * $NetBSD: lockd.c,v 1.7 2000/08/12 18:08:44 thorpej Exp $
 * $FreeBSD: src/usr.sbin/rpc.lockd/lockd.c,v 1.6 2001/03/19 12:50:09 alfred Exp $
 */

/*
 * main() function for NFS lock daemon.  Most of the code in this
 * file was generated by running rpcgen /usr/include/rpcsvc/nlm_prot.x.
 *
 * The actual program logic is in the file lock_proc.c
 */

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <libutil.h>
#include <netconfig.h>
#include <netdb.h>

#include <rpc/rpc.h>
#include <rpc/rpc_com.h>
#include <rpcsvc/sm_inter.h>

#include "lockd.h"
#include <rpcsvc/nlm_prot.h>

int		debug_level = 0;	/* 0 = no debugging syslog() calls */
int		_rpcsvcdirty = 0;

int grace_expired;
char **hosts, *svcport_str = NULL;
int nhosts = 0;
int xcreated = 0;

#ifndef IPPORT_MAX
#define IPPORT_MAX	65535
#endif

void	create_service(struct netconfig *);

void	nlm_prog_0(struct svc_req *, SVCXPRT *);
void	nlm_prog_1(struct svc_req *, SVCXPRT *);
void	nlm_prog_3(struct svc_req *, SVCXPRT *);
void	nlm_prog_4(struct svc_req *, SVCXPRT *);
void	out_of_mem(void) __dead2;

static void	usage(void) __dead2;

void	sigalarm_handler(void);

int
main(int argc, char **argv)
{
	int ch, i, s;
	void *nc_handle;
	char *endptr, **hosts_bak;
	struct sigaction sigalarm;
	int grace_period = 30;
	struct netconfig *nconf;
	int have_v6 = 1;
	int maxrec = RPC_MAXDATASIZE;
	in_port_t svcport = 0;

	while ((ch = getopt(argc, argv, "d:g:h:p:")) != (-1)) {
		switch (ch) {
		case 'd':
			debug_level = atoi(optarg);
			if (!debug_level) {
				usage();
				/* NOTREACHED */
			}
			break;
		case 'g':
			grace_period = atoi(optarg);
			if (!grace_period) {
				usage();
				/* NOTREACHED */
			}
			break;
		case 'h':
			++nhosts;
			hosts_bak = hosts;
			hosts_bak = realloc(hosts, nhosts * sizeof(char *));
			if (hosts_bak == NULL) {
				if (hosts != NULL) {
					for (i = 0; i < nhosts; i++)
						free(hosts[i]);
					free(hosts);
					out_of_mem();
				}
			}
			hosts = hosts_bak;
			hosts[nhosts - 1] = strdup(optarg);
			if (hosts[nhosts - 1] == NULL) {
				for (i = 0; i < (nhosts - 1); i++)
					free(hosts[i]);
				free(hosts);
				out_of_mem();
			}
			break;
		case 'p':
			endptr = NULL;
			svcport = (in_port_t)strtoul(optarg, &endptr, 10);
			if (endptr == NULL || *endptr != '\0' ||
			    svcport == 0 || svcport >= IPPORT_MAX)
				usage();
			svcport_str = strdup(optarg);
			break;
		default:
		case '?':
			usage();
			/* NOTREACHED */
		}
	}
	if (geteuid()) { /* This command allowed only to root */
		fprintf(stderr, "Sorry. You are not superuser\n");
		exit(1);
        }

	rpcb_unset(NLM_PROG, NLM_SM, NULL);
	rpcb_unset(NLM_PROG, NLM_VERS, NULL);
	rpcb_unset(NLM_PROG, NLM_VERSX, NULL);
	rpcb_unset(NLM_PROG, NLM_VERS4, NULL);

	/*
	 * Check if IPv6 support is present.
	 */
	s = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (s < 0)
		have_v6 = 0;
	else
		close(s);

	rpc_control(RPC_SVC_CONNMAXREC_SET, &maxrec);

	/*
	 * If no hosts were specified, add a wildcard entry to bind to
	 * INADDR_ANY. Otherwise make sure 127.0.0.1 and ::1 are added to the
	 * list.
	 */
	if (nhosts == 0) {
		hosts = malloc(sizeof(char**));
		if (hosts == NULL)
			out_of_mem();

		hosts[0] = "*";
		nhosts = 1;
	} else {
		hosts_bak = hosts;
		if (have_v6) {
			hosts_bak = realloc(hosts, (nhosts + 2) *
			    sizeof(char *));
			if (hosts_bak == NULL) {
				for (i = 0; i < nhosts; i++)
					free(hosts[i]);
				free(hosts);
				out_of_mem();
			} else
				hosts = hosts_bak;

			nhosts += 2;
			hosts[nhosts - 2] = "::1";
		} else {
			hosts_bak = realloc(hosts, (nhosts + 1) * sizeof(char *));
			if (hosts_bak == NULL) {
				for (i = 0; i < nhosts; i++)
					free(hosts[i]);

				free(hosts);
				out_of_mem();
			} else {
				nhosts += 1;
				hosts = hosts_bak;
			}
		}
		hosts[nhosts - 1] = "127.0.0.1";
	}

	nc_handle = setnetconfig();
	while ((nconf = getnetconfig(nc_handle))) {
		/* We want to listen only on udp6, tcp6, udp, tcp transports */
		if (nconf->nc_flag & NC_VISIBLE) {
			/* Skip if there's no IPv6 support */
			if (have_v6 == 0 && strcmp(nconf->nc_protofmly, "inet6") == 0) {
				/* DO NOTHING */
			} else {
				create_service(nconf);
			}
		}
	}
	endnetconfig(nc_handle);

	/*
	 * Note that it is NOT sensible to run this program from inetd - the
	 * protocol assumes that it will run immediately at boot time.
	 */
	if (daemon(0, debug_level > 0)) {
		err(1, "cannot fork");
		/* NOTREACHED */
	}

	openlog("rpc.lockd", 0, LOG_DAEMON);
	if (debug_level)
		syslog(LOG_INFO, "Starting, debug level %d", debug_level);
	else
		syslog(LOG_INFO, "Starting");

	sigalarm.sa_handler = (sig_t) sigalarm_handler;
	sigemptyset(&sigalarm.sa_mask);
	sigalarm.sa_flags = SA_RESETHAND; /* should only happen once */
	sigalarm.sa_flags |= SA_RESTART;
	if (sigaction(SIGALRM, &sigalarm, NULL) != 0) {
		syslog(LOG_WARNING, "sigaction(SIGALRM) failed: %s",
		    strerror(errno));
		exit(1);
	}
	grace_expired = 0;
	alarm(grace_period);

	svc_run();		/* Should never return */
	exit(1);
}

/*
 * This routine creates and binds sockets on the appropriate
 * addresses. It gets called one time for each transport and
 * registrates the service with rpcbind on that trasport.
 */
void
create_service(struct netconfig *nconf)
{
	struct addrinfo hints, *res = NULL;
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;
	struct __rpc_sockinfo si;
	struct netbuf servaddr;
	SVCXPRT	*transp = NULL;
	int aicode;
	int fd;
	int nhostsbak;
	int r;
	int registered = 0;
	u_int32_t host_addr[4];  /* IPv4 or IPv6 */

	if ((nconf->nc_semantics != NC_TPI_CLTS) &&
	    (nconf->nc_semantics != NC_TPI_COTS) &&
	    (nconf->nc_semantics != NC_TPI_COTS_ORD))
		return;	/* not my type */

	/*
	 * XXX - using RPC library internal functions.
	 */
	if (!__rpc_nconf2sockinfo(nconf, &si)) {
		syslog(LOG_ERR, "cannot get information for %s",
		    nconf->nc_netid);
		return;
	}

	/* Get rpc.statd's address on this transport */
	memset(&hints, 0, sizeof hints);
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = si.si_af;
	hints.ai_socktype = si.si_socktype;
	hints.ai_protocol = si.si_proto;

	/*
	 * Bind to specific IPs if asked to
	 */
	nhostsbak = nhosts;
	while (nhostsbak > 0) {
		--nhostsbak;

		/*
		 * XXX - using RPC library internal functions.
		 */
		if ((fd = __rpc_nconf2fd(nconf)) < 0) {
			syslog(LOG_ERR, "cannot create socket for %s",
			    nconf->nc_netid);
			continue;
		}

		switch (hints.ai_family) {
			case AF_INET:
				if (inet_pton(AF_INET, hosts[nhostsbak],
				    host_addr) == 1) {
					hints.ai_flags &= AI_NUMERICHOST;
				} else {
					/*
					 * Skip if we have an AF_INET6 address.
					 */
					if (inet_pton(AF_INET6, hosts[nhostsbak],
					    host_addr) == 1) {
						close(fd);
						continue;
					}
				}
				break;
			case AF_INET6:
				if (inet_pton(AF_INET6, hosts[nhostsbak],
				    host_addr) == 1) {
					hints.ai_flags &= AI_NUMERICHOST;
				} else {
					/*
					 * Skip if we have an AF_INET address.
					 */
					if (inet_pton(AF_INET, hosts[nhostsbak],
					    host_addr) == 1) {
						close(fd);
						continue;
					}
				}
				break;
			default:
				break;
		}

		/*
		 * If no hosts were specified, just bind to INADDR_ANY
		 */
		if (strcmp("*", hosts[nhostsbak]) == 0) {
			if (svcport_str == NULL) {
				res = malloc(sizeof(struct addrinfo));
				if (res == NULL)
					out_of_mem();
				res->ai_flags = hints.ai_flags;
				res->ai_family = hints.ai_family;
				res->ai_protocol = hints.ai_protocol;
				switch (res->ai_family) {
					case AF_INET:
						sin = malloc(sizeof(struct sockaddr_in));
						if (sin == NULL)
							out_of_mem();
						sin->sin_family = AF_INET;
						sin->sin_port = htons(0);
						sin->sin_addr.s_addr = htonl(INADDR_ANY);
						res->ai_addr = (struct sockaddr*) sin;
						res->ai_addrlen = (socklen_t)
						    sizeof(res->ai_addr);
						break;
					case AF_INET6:
						sin6 = malloc(sizeof(struct sockaddr_in6));
						if (sin6 == NULL)
							out_of_mem();
						sin6->sin6_family = AF_INET6;
						sin6->sin6_port = htons(0);
						sin6->sin6_addr = in6addr_any;
						res->ai_addr = (struct sockaddr*) sin6;
						res->ai_addrlen = (socklen_t) sizeof(res->ai_addr);
						break;
					default:
						break;
				}
			} else {
				if ((aicode = getaddrinfo(NULL, svcport_str,
				    &hints, &res)) != 0) {
					syslog(LOG_ERR,
					    "cannot get local address for %s: %s",
					    nconf->nc_netid,
					    gai_strerror(aicode));
					continue;
				}
			}
		} else {
			if ((aicode = getaddrinfo(hosts[nhostsbak], svcport_str,
			    &hints, &res)) != 0) {
				syslog(LOG_ERR,
				    "cannot get local address for %s: %s",
				    nconf->nc_netid, gai_strerror(aicode));
				continue;
			}
		}

		r = bindresvport_sa(fd, res->ai_addr);
		if (r != 0) {
			syslog(LOG_ERR, "bindresvport_sa: %m");
			exit(1);
		}

		transp = svc_tli_create(fd, nconf, NULL,
		    RPC_MAXDATASIZE, RPC_MAXDATASIZE);

		if (transp != NULL) {
			if (!svc_reg(transp, NLM_PROG, NLM_SM, nlm_prog_0,
			    NULL))
				syslog(LOG_ERR,
				    "can't register %s NLM_PROG, NLM_SM service",
				    nconf->nc_netid);

			if (!svc_reg(transp, NLM_PROG, NLM_VERS, nlm_prog_1,
			    NULL))
				syslog(LOG_ERR,
				    "can't register %s NLM_PROG, NLM_VERS service",
				    nconf->nc_netid);

			if (!svc_reg(transp, NLM_PROG, NLM_VERSX, nlm_prog_3,
			    NULL))
				syslog(LOG_ERR,
				    "can't register %s NLM_PROG, NLM_VERSX service",
				    nconf->nc_netid);

			if (!svc_reg(transp, NLM_PROG, NLM_VERS4, nlm_prog_4,
			    NULL))
				syslog(LOG_ERR,
				    "can't register %s NLM_PROG, NLM_VERS4 service",
				    nconf->nc_netid);

		} else
			syslog(LOG_WARNING, "can't create %s services",
			    nconf->nc_netid);

		if (registered == 0) {
			registered = 1;
			memset(&hints, 0, sizeof hints);
			hints.ai_flags = AI_PASSIVE;
			hints.ai_family = si.si_af;
			hints.ai_socktype = si.si_socktype;
			hints.ai_protocol = si.si_proto;

			if (svcport_str == NULL) {
				svcport_str = malloc(NI_MAXSERV * sizeof(char));
				if (svcport_str == NULL)
					out_of_mem();

				if (getnameinfo(res->ai_addr,
				    res->ai_addr->sa_len, NULL, NI_MAXHOST,
				    svcport_str, NI_MAXSERV * sizeof(char),
				    NI_NUMERICHOST | NI_NUMERICSERV))
					errx(1, "Cannot get port number");
			}

			if((aicode = getaddrinfo(NULL, svcport_str, &hints,
			    &res)) != 0) {
				syslog(LOG_ERR, "cannot get local address: %s",
				    gai_strerror(aicode));
				exit(1);
			}

			servaddr.buf = malloc(res->ai_addrlen);
			memcpy(servaddr.buf, res->ai_addr, res->ai_addrlen);
			servaddr.len = res->ai_addrlen;

			rpcb_set(NLM_PROG, NLM_SM, nconf, &servaddr);
			rpcb_set(NLM_PROG, NLM_VERS, nconf, &servaddr);
			rpcb_set(NLM_PROG, NLM_VERSX, nconf, &servaddr);
			rpcb_set(NLM_PROG, NLM_VERS4, nconf, &servaddr);

			xcreated++;
			freeaddrinfo(res);
		}
	} /* end while */
}

void
sigalarm_handler(void)
{

	grace_expired = 1;
}

static void
usage(void)
{
	errx(1, "usage: rpc.lockd [-d <debuglevel>]"
	    " [-g <grace period>] [-h <bindip>] [-p <port>]");
}

/*
 * Out of memory, fatal
 */
void
out_of_mem(void)
{
	syslog(LOG_ERR, "out of memory");
	exit(2);
}
