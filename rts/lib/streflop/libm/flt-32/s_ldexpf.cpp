/* See the import.pl script for potential modifications */
/* s_ldexpf.c -- StreflopSimple version of s_ldexp.c.
 * Conversion to StreflopSimple by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#if defined(LIBM_SCCS) && !defined(lint)
static char rcsid[] = "$NetBSD: s_ldexpf.c,v 1.3f 1995/05/10 20:47:42 jtc Exp $";
#endif

#include "SMath.h"
#include "math_private.h"
#include "../streflop_libm_bridge.h"

namespace streflop_libm {
#ifdef __STDC__
	StreflopSimple __ldexpf(StreflopSimple value, int exp)
#else
	StreflopSimple __ldexpf(value, exp)
	StreflopSimple value; int exp;
#endif
{
	if(!__finitef(value)||value==(StreflopSimple)0.0f) return value;
	value = __scalbnf(value,exp);
	if(!__finitef(value)||value==(StreflopSimple)0.0f) __set_errno (ERANGE);
	return value;
}
INTDEF(__ldexpf)
weak_alias (__ldexpf, ldexpf)
}
