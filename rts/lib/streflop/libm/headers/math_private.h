/* See the import.pl script for potential modifications */
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

/*
 * from: @(#)fdlibm.h 5.1 93/09/24
 */

#ifndef _MATH_PRIVATE_H_
#define _MATH_PRIVATE_H_

/* import.pl: Skipped the macro definitions, keep only the declarations, converted to use streflop types (aliases or wrappers) */
#include "../streflop_libm_bridge.h"

namespace streflop_libm {

#ifdef LIBM_COMPILING_FLT32
#define __sqrtf __ieee754_sqrtf
#define fabsf __fabsf
#define copysignf __copysignf
extern StreflopSimple __log1pf(StreflopSimple x);
extern StreflopSimple __fabsf(StreflopSimple x);
extern StreflopSimple __atanf(StreflopSimple x);
extern StreflopSimple __expm1f(StreflopSimple x);
extern int __isinff(StreflopSimple x);
extern StreflopSimple __rintf(StreflopSimple x);
extern StreflopSimple __cosf(StreflopSimple x);
extern void __sincosf (StreflopSimple x, StreflopSimple *sinx, StreflopSimple *cosx);
extern StreflopSimple __floorf(StreflopSimple x);
extern StreflopSimple __scalbnf (StreflopSimple x, int n);
extern StreflopSimple __frexpf(StreflopSimple x, int *eptr);
extern StreflopSimple __ldexpf(StreflopSimple value, int exp);
extern int __finitef(StreflopSimple x);
#endif

#ifdef LIBM_COMPILING_DBL64
#define __sqrt __ieee754_sqrt
#define fabs __fabs
#define copysign __copysign
extern StreflopDouble __log1p(StreflopDouble x);
extern StreflopDouble __fabs(StreflopDouble x);
extern StreflopDouble atan(StreflopDouble x);
extern StreflopDouble __expm1(StreflopDouble x);
extern int __isinf(StreflopDouble x);
extern StreflopDouble __rint(StreflopDouble x);
extern StreflopDouble __cos(StreflopDouble x);
extern void __sincos (StreflopDouble x, StreflopDouble *sinx, StreflopDouble *cosx);
extern StreflopDouble __floor(StreflopDouble x);
extern StreflopDouble __scalbn(StreflopDouble x, int n);
extern StreflopDouble __frexp(StreflopDouble x, int *eptr);
extern StreflopDouble __ldexp(StreflopDouble value, int exp);
extern int __finite(StreflopDouble x);
#endif

#ifdef LIBM_COMPILING_LDBL96
#if defined(Extended)
#define fabsl __fabsl
extern Extended __cosl(Extended x);
extern Extended __sinl(Extended x);
extern Extended __fabsl(Extended x);
#endif
#endif

#ifdef LIBM_COMPILING_DBL64
extern StreflopDouble __ieee754_sqrt (StreflopDouble);
extern StreflopDouble __ieee754_acos (StreflopDouble);
extern StreflopDouble __ieee754_acosh (StreflopDouble);
extern StreflopDouble __ieee754_log (StreflopDouble);
extern StreflopDouble __ieee754_atanh (StreflopDouble);
extern StreflopDouble __ieee754_asin (StreflopDouble);
extern StreflopDouble __ieee754_atan2 (StreflopDouble,StreflopDouble);
extern StreflopDouble __ieee754_exp (StreflopDouble);
extern StreflopDouble __ieee754_exp2 (StreflopDouble);
extern StreflopDouble __ieee754_exp10 (StreflopDouble);
extern StreflopDouble __ieee754_cosh (StreflopDouble);
extern StreflopDouble __ieee754_fmod (StreflopDouble,StreflopDouble);
extern StreflopDouble __ieee754_pow (StreflopDouble,StreflopDouble);
extern StreflopDouble __ieee754_lgamma_r (StreflopDouble,int *);
extern StreflopDouble __ieee754_gamma_r (StreflopDouble,int *);
extern StreflopDouble __ieee754_lgamma (StreflopDouble);
extern StreflopDouble __ieee754_gamma (StreflopDouble);
extern StreflopDouble __ieee754_log10 (StreflopDouble);
extern StreflopDouble __ieee754_log2 (StreflopDouble);
extern StreflopDouble __ieee754_sinh (StreflopDouble);
extern StreflopDouble __ieee754_hypot (StreflopDouble,StreflopDouble);
extern StreflopDouble __ieee754_j0 (StreflopDouble);
extern StreflopDouble __ieee754_j1 (StreflopDouble);
extern StreflopDouble __ieee754_y0 (StreflopDouble);
extern StreflopDouble __ieee754_y1 (StreflopDouble);
extern StreflopDouble __ieee754_jn (int,StreflopDouble);
extern StreflopDouble __ieee754_yn (int,StreflopDouble);
extern StreflopDouble __ieee754_remainder (StreflopDouble,StreflopDouble);
extern int32_t __ieee754_rem_pio2 (StreflopDouble,StreflopDouble*);
extern StreflopDouble __ieee754_scalb (StreflopDouble,StreflopDouble);
#endif

/* fdlibm kernel function */
#ifdef LIBM_COMPILING_DBL64
extern StreflopDouble __kernel_standard (StreflopDouble,StreflopDouble,int);
extern StreflopDouble __kernel_sin (StreflopDouble,StreflopDouble,int);
extern StreflopDouble __kernel_cos (StreflopDouble,StreflopDouble);
extern StreflopDouble __kernel_tan (StreflopDouble,StreflopDouble,int);
extern int    __kernel_rem_pio2 (StreflopDouble*,StreflopDouble*,int,int,int, const int32_t*);
#endif

/* internal functions.  */
#ifdef LIBM_COMPILING_DBL64
extern StreflopDouble __copysign (StreflopDouble x, StreflopDouble __y);
#endif

#if 0
#ifdef LIBM_COMPILING_DBL64
extern inline StreflopDouble __copysign (StreflopDouble x, StreflopDouble y)
{ return __builtin_copysign (x, y); }
#endif
#endif

/* ieee style elementary StreflopSimple functions */
#ifdef LIBM_COMPILING_FLT32
extern StreflopSimple __ieee754_sqrtf (StreflopSimple);
extern StreflopSimple __ieee754_acosf (StreflopSimple);
extern StreflopSimple __ieee754_acoshf (StreflopSimple);
extern StreflopSimple __ieee754_logf (StreflopSimple);
extern StreflopSimple __ieee754_atanhf (StreflopSimple);
extern StreflopSimple __ieee754_asinf (StreflopSimple);
extern StreflopSimple __ieee754_atan2f (StreflopSimple,StreflopSimple);
extern StreflopSimple __ieee754_expf (StreflopSimple);
extern StreflopSimple __ieee754_exp2f (StreflopSimple);
extern StreflopSimple __ieee754_exp10f (StreflopSimple);
extern StreflopSimple __ieee754_coshf (StreflopSimple);
extern StreflopSimple __ieee754_fmodf (StreflopSimple,StreflopSimple);
extern StreflopSimple __ieee754_powf (StreflopSimple,StreflopSimple);
extern StreflopSimple __ieee754_lgammaf_r (StreflopSimple,int *);
extern StreflopSimple __ieee754_gammaf_r (StreflopSimple,int *);
extern StreflopSimple __ieee754_lgammaf (StreflopSimple);
extern StreflopSimple __ieee754_gammaf (StreflopSimple);
extern StreflopSimple __ieee754_log10f (StreflopSimple);
extern StreflopSimple __ieee754_log2f (StreflopSimple);
extern StreflopSimple __ieee754_sinhf (StreflopSimple);
extern StreflopSimple __ieee754_hypotf (StreflopSimple,StreflopSimple);
extern StreflopSimple __ieee754_j0f (StreflopSimple);
extern StreflopSimple __ieee754_j1f (StreflopSimple);
extern StreflopSimple __ieee754_y0f (StreflopSimple);
extern StreflopSimple __ieee754_y1f (StreflopSimple);
extern StreflopSimple __ieee754_jnf (int,StreflopSimple);
extern StreflopSimple __ieee754_ynf (int,StreflopSimple);
extern StreflopSimple __ieee754_remainderf (StreflopSimple,StreflopSimple);
extern int32_t __ieee754_rem_pio2f (StreflopSimple,StreflopSimple*);
extern StreflopSimple __ieee754_scalbf (StreflopSimple,StreflopSimple);
#endif


/* StreflopSimple versions of fdlibm kernel functions */
#ifdef LIBM_COMPILING_FLT32
extern StreflopSimple __kernel_sinf (StreflopSimple,StreflopSimple,int);
extern StreflopSimple __kernel_cosf (StreflopSimple,StreflopSimple);
extern StreflopSimple __kernel_tanf (StreflopSimple,StreflopSimple,int);
extern int   __kernel_rem_pio2f (StreflopSimple*,StreflopSimple*,int,int,int, const int32_t*);
#endif

/* internal functions.  */
#ifdef LIBM_COMPILING_FLT32
extern StreflopSimple __copysignf (StreflopSimple x, StreflopSimple __y);
#endif

#if 0
#ifdef LIBM_COMPILING_FLT32
extern inline StreflopSimple __copysignf (StreflopSimple x, StreflopSimple y)
{ return __builtin_copysignf (x, y); }
#endif
#endif

#if defined(Extended)
/* ieee style elementary Extended functions */
#ifdef LIBM_COMPILING_LDBL96
extern Extended __ieee754_sqrtl (Extended);
extern Extended __ieee754_acosl (Extended);
extern Extended __ieee754_acoshl (Extended);
extern Extended __ieee754_logl (Extended);
extern Extended __ieee754_atanhl (Extended);
extern Extended __ieee754_asinl (Extended);
extern Extended __ieee754_atan2l (Extended,Extended);
extern Extended __ieee754_expl (Extended);
extern Extended __ieee754_exp2l (Extended);
extern Extended __ieee754_exp10l (Extended);
extern Extended __ieee754_coshl (Extended);
extern Extended __ieee754_fmodl (Extended,Extended);
extern Extended __ieee754_powl (Extended,Extended);
extern Extended __ieee754_lgammal_r (Extended,int *);
extern Extended __ieee754_gammal_r (Extended,int *);
extern Extended __ieee754_lgammal (Extended);
extern Extended __ieee754_gammal (Extended);
extern Extended __ieee754_log10l (Extended);
extern Extended __ieee754_log2l (Extended);
extern Extended __ieee754_sinhl (Extended);
extern Extended __ieee754_hypotl (Extended,Extended);
extern Extended __ieee754_j0l (Extended);
extern Extended __ieee754_j1l (Extended);
extern Extended __ieee754_y0l (Extended);
extern Extended __ieee754_y1l (Extended);
extern Extended __ieee754_jnl (int,Extended);
extern Extended __ieee754_ynl (int,Extended);
extern Extended __ieee754_remainderl (Extended,Extended);
extern int   __ieee754_rem_pio2l (Extended,Extended*);
extern Extended __ieee754_scalbl (Extended,Extended);
#endif

/* Extended versions of fdlibm kernel functions */
#ifdef LIBM_COMPILING_LDBL96
extern Extended __kernel_sinl (Extended,Extended,int);
extern Extended __kernel_cosl (Extended,Extended);
extern Extended __kernel_tanl (Extended,Extended,int);
extern void __kernel_sincosl (Extended,Extended,
			      Extended *,Extended *, int);
extern int   __kernel_rem_pio2l (Extended*,Extended*,int,int,
				 int,const int*);
#endif

#ifndef NO_LONG_DOUBLE
/* prototypes required to compile the ldbl-96 support without warnings */
#ifdef LIBM_COMPILING_LDBL96
extern int __finitel (Extended);
extern int __ilogbl (Extended);
extern int __isinfl (Extended);
extern int __isnanl (Extended);
extern Extended __atanl (Extended);
extern Extended __copysignl (Extended, Extended);
extern Extended __expm1l (Extended);
extern Extended __floorl (Extended);
extern Extended __frexpl (Extended, int *);
extern Extended __ldexpl (Extended, int);
extern Extended __log1pl (Extended);
extern Extended __nanl (const char *);
extern Extended __rintl (Extended);
extern Extended __scalbnl (Extended, int);
extern Extended __sqrtl (Extended x);
extern Extended fabsl (Extended x);
extern void __sincosl (Extended, Extended *, Extended *);
extern Extended __logbl (Extended x);
extern Extended __significandl (Extended x);
#endif

#if 0
#ifdef LIBM_COMPILING_LDBL96
extern inline Extended __copysignl (Extended x, Extended y)
{ return __builtin_copysignl (x, y); }
#endif
#endif

#endif

#endif

/* Prototypes for functions of the IBM Accurate Mathematical Library.  */
#ifdef LIBM_COMPILING_DBL64
extern StreflopDouble __exp1 (StreflopDouble __x, StreflopDouble __xx, StreflopDouble __error);
extern StreflopDouble __sin (StreflopDouble __x);
extern StreflopDouble __cos (StreflopDouble __x);
extern int __branred (StreflopDouble __x, StreflopDouble *__a, StreflopDouble *__aa);
extern void __doasin (StreflopDouble __x, StreflopDouble __dx, StreflopDouble __v[]);
extern void __dubsin (StreflopDouble __x, StreflopDouble __dx, StreflopDouble __v[]);
extern void __dubcos (StreflopDouble __x, StreflopDouble __dx, StreflopDouble __v[]);
extern StreflopDouble __halfulp (StreflopDouble __x, StreflopDouble __y);
extern StreflopDouble __sin32 (StreflopDouble __x, StreflopDouble __res, StreflopDouble __res1);
extern StreflopDouble __cos32 (StreflopDouble __x, StreflopDouble __res, StreflopDouble __res1);
extern StreflopDouble __mpsin (StreflopDouble __x, StreflopDouble __dx);
extern StreflopDouble __mpcos (StreflopDouble __x, StreflopDouble __dx);
extern StreflopDouble __mpsin1 (StreflopDouble __x);
extern StreflopDouble __mpcos1 (StreflopDouble __x);
extern StreflopDouble __slowexp (StreflopDouble __x);
extern StreflopDouble __slowpow (StreflopDouble __x, StreflopDouble __y, StreflopDouble __z);
extern void __docos (StreflopDouble __x, StreflopDouble __dx, StreflopDouble __v[]);
#endif

}

#endif /* _MATH_PRIVATE_H_ */
