/* See the import.pl script for potential modifications */
/* e_atanhf.c -- StreflopSimple version of e_atanh.c.
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
static char rcsid[] = "$NetBSD: e_atanhf.c,v 1.4f 1995/05/10 20:44:56 jtc Exp $";
#endif

#include "SMath.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const StreflopSimple one = 1.0f, huge = 1e30f;
#else
static StreflopSimple one = 1.0f, huge = 1e30f;
#endif

#ifdef __STDC__
static const StreflopSimple zero = 0.0f;
#else
static StreflopSimple zero = 0.0f;
#endif

#ifdef __STDC__
	StreflopSimple __ieee754_atanhf(StreflopSimple x)
#else
	StreflopSimple __ieee754_atanhf(x)
	StreflopSimple x;
#endif
{
	StreflopSimple t;
	int32_t hx,ix;
	GET_FLOAT_WORD(hx,x);
	ix = hx&0x7fffffff;
	if (ix>0x3f800000) 		/* |x|>1 */
	    return (x-x)/(x-x);
	if(ix==0x3f800000) 
	    return x/zero;
	if(ix<0x31800000&&(huge+x)>zero) return x;	/* x<2**-28 */
	SET_FLOAT_WORD(x,ix);
	if(ix<0x3f000000) {		/* x < 0.5f */
	    t = x+x;
	    t = (StreflopSimple)0.5f*__log1pf(t+t*x/(one-x));
	} else 
	    t = (StreflopSimple)0.5f*__log1pf((x+x)/(one-x));
	if(hx>=0) return t; else return -t;
}
}
