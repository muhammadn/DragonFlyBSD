/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 *
 * $FreeBSD: head/lib/msun/i387/s_significandf.S 217108 2011-01-07 16:13:12Z kib $
 * $NetBSD: s_significandf.S,v 1.3 1995/05/09 00:24:07 jtc Exp $
 */

#include <machine/asm.h>

ENTRY(significandf)
	flds	4(%esp)
	fxtract
	fstp	%st(1)
	ret
END(significandf)

	.section .note.GNU-stack,"",%progbits
