/*-
 * Copyright (C) 2003 David Xu <davidxu@freebsd.org>
 * Copyright (c) 2001,2003 Daniel Eischen <deischen@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 * $DragonFly: src/lib/libthread_xu/arch/i386/i386/pthread_md.c,v 1.3 2005/02/22 14:56:22 davidxu Exp $
 */

#include <sys/types.h>
#include <sys/tls.h>
#include <stdlib.h>

#include "pthread_md.h"

struct tcb *
_tcb_ctor(struct pthread *thread, int initial)
{
	struct tcb *tcb;

	if ((tcb = malloc(sizeof(struct tcb))) == NULL)
		return (NULL);
	tcb->tcb_self = tcb;
	tcb->tcb_dtv = NULL;
	tcb->tcb_thread = thread;
	tcb->tcb_seg = -1;
	return (tcb);
}

void
_tcb_set(struct tcb *tcb)
{
	struct tls_info info;
	int seg;

	info.base = tcb;
	info.size = sizeof(*tcb);
	seg = tcb->tcb_seg = sys_set_tls_area(0, &info, sizeof(info));
	__asm __volatile("movl %0, %%gs" : : "r" (seg));
}

void
_tcb_dtor(struct tcb *tcb)
{
	free(tcb);
}
