/* $FreeBSD: src/sys/dev/bktr/bktr_os.h,v 1.7 2003/12/01 19:03:50 truckman Exp $ */
/* $DragonFly: src/sys/dev/video/bktr/bktr_os.h,v 1.4 2004/05/15 17:54:13 joerg Exp $ */

/*
 * This is part of the Driver for Video Capture Cards (Frame grabbers)
 * and TV Tuner cards using the Brooktree Bt848, Bt848A, Bt849A, Bt878, Bt879
 * chipset.
 * Copyright Roger Hardiman and Amancio Hasty.
 *
 * bktr_os : This has all the Operating System dependant code.
 *
 */

/*
 * 1. Redistributions of source code must retain the 
 * Copyright (c) 1997 Amancio Hasty, 1999 Roger Hardiman
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Amancio Hasty and
 *      Roger Hardiman
 * 4. The name of the author may not be used to endorse or promote products 
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.	IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


/******************************/
/* *** Memory Allocation  *** */
/******************************/
vm_offset_t     get_bktr_mem( int unit, unsigned size );

/************************************/
/* *** Interrupt Enable/Disable *** */
/************************************/
#define DECLARE_INTR_MASK(s)	intrmask_t s
#define DISABLE_INTR(s)		s=spltty()
#define ENABLE_INTR(s)		splx(s)

#ifdef USE_VBIMUTEX
#define LOCK_VBI(bktr)		mtx_lock(&bktr->vbimutex)
#define UNLOCK_VBI(bktr)	mtx_unlock(&bktr->vbimutex)
#else
#define LOCK_VBI(bktr)
#define UNLOCK_VBI(bktr)
#endif
