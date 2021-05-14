/* See the import.pl script for potential modifications */
/* s_logbf.c -- StreflopSimple version of s_logb.c.
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
static char rcsid[] = "$NetBSD: s_logbf.c,v 1.4f 1995/05/10 20:47:51 jtc Exp $";
#endif

#include "SMath.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
	StreflopSimple __logbf(StreflopSimple x)
#else
	StreflopSimple __logbf(x)
	StreflopSimple x;
#endif
{
	int32_t ix;
	GET_FLOAT_WORD(ix,x);
	ix &= 0x7fffffff;			/* high |x| */
	if(ix==0) return (StreflopSimple)-1.0f/fabsf(x);
	if(ix>=0x7f800000) return x*x;
	if((ix>>=23)==0) 			/* IEEE 754 logb */
		return -126.0f; 
	else
		return (StreflopSimple) (ix-127); 
}
weak_alias (__logbf, logbf)
}
