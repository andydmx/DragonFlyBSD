/*	@(#)wwclreol.c	8.1 (Berkeley) 6/6/93	*/
/*	$NetBSD: wwclreol.c,v 1.7 2003/08/07 11:17:37 agc Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Edward Wang at The University of California, Berkeley.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "ww.h"
#include "tt.h"

/*
 * Clear w to the end of line.
 * If cleared is true, then the screen line has already been cleared.
 */
void
wwclreol1(struct ww *w, int row, int col, char cleared)
{
	int i;

	/*
	 * Clear the buffer right off
	 */
	{
		union ww_char *buf;

		buf = &w->ww_buf[row][col];
		for (i = w->ww_b.r - col; --i >= 0;)
			buf++->c_w = ' ';
	}

	/*
	 * If can't see it, just return.
	 */
	if (row < w->ww_i.t || row >= w->ww_i.b
	    || w->ww_i.r <= 0 || w->ww_i.r <= col)
		return;

	if (col < w->ww_i.l)
		col = w->ww_i.l;

	/*
	 * Now fix wwns.
	 */
	{
		union ww_char *s;
		unsigned char *smap;
		char *win;

		i = col;
		smap = &wwsmap[row][i];
		s = &wwns[row][i];
		win = &w->ww_win[row][i];
		for (i = w->ww_i.r - i; --i >= 0;)
			if (*smap++ == w->ww_index)
				s++->c_w = ' ' | *win++ << WWC_MSHIFT;
			else
				s++, win++;
	}
	if (!cleared)
		wwtouched[row] |= WWU_TOUCHED;
}
